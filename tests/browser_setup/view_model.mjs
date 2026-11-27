// Offline presentation candidate; no DOM, network, crypto, credentials or device authorization.
export function decodeSetupReply(bytes) {
  if (!(bytes instanceof Uint8Array) || bytes.length !== 11 || bytes[0] !== 1) return null;
  const code = bytes[1], phase = bytes[2];
  if (code < 1 || code > 6 || phase > 3) return null;
  let trial = "";
  for (let i = 3; i < 11; i++) trial += bytes[i].toString(16).padStart(2, "0");
  const active = phase === 1 || phase === 2;
  if (active === (trial === "0000000000000000")) return null;
  return { code, phase, trial: active ? trial : null };
}
export function createSetupView() {
  // Presentation model only. The future trusted transport owns authentication.
  // Never use this model as authorization for a device-side command.
  let mode = "locked", phase = 0, trial = null, pending = null;
  let opened = null, last = null, verified = null, serial = 0;
  let result = "locked";
  function stop(reason) {
    if (mode !== "open") return;
    mode = pending && pending.action === "save" ? "uncertain" : "closed";
    result = mode === "uncertain" ? "save-outcome-unknown" : reason;
    pending = null; verified = null; trial = null;
  }
  function tick(now) {
    if (!Number.isSafeInteger(now) || now < 0 || (last !== null && now < last)) {
      stop("clock-error"); return false;
    }
    last = now;
    if (mode === "open" && (now - opened >= 300000 ||
        (pending && now - pending.started >= 2000))) stop("expired");
    return mode === "open";
  }
  function authenticated(now) {
    // Only an independently trusted adapter may call this after authentication.
    // This is not a TLS or Security 2 check.
    if (mode !== "locked" || !Number.isSafeInteger(now) || now < 0 ||
        (last !== null && now < last)) return false;
    mode = "open"; opened = last = now; result = "ready"; return true;
  }
  function snapshot(now) {
    tick(now);
    const ready = mode === "open" && pending === null;
    return Object.freeze({ mode, phase, result, busy: pending !== null,
      canStage: ready && phase === 0, canStatus: ready, canCancel: ready,
      canSave: ready && phase === 2 && trial !== null && verified !== null &&
        now - verified < 2000 });
  }
  function begin(action, now, confirmed = false) {
    const s = snapshot(now);
    if (!((action === "stage" && s.canStage) || (action === "status" && s.canStatus) ||
          (action === "cancel" && s.canCancel) ||
          (action === "save" && s.canSave && confirmed === true))) return null;
    if (serial === Number.MAX_SAFE_INTEGER) { stop("ticket-exhausted"); return null; }
    pending = { id: ++serial, action, started: now };
    verified = null; result = "waiting";
    return pending.id;
  }
  function receive(id, bytes, now) {
    if (!tick(now) || !pending || id !== pending.id) return false;
    const r = decodeSetupReply(bytes), action = pending.action;
    if (!r) { stop("invalid-reply"); return false; }
    const finalSave = action === "save" && (r.code === 4 || r.code === 5) && r.phase === 3;
    const finalCancel = action === "cancel" && r.code === 6 && (r.phase === 0 || r.phase === 3);
    if (finalSave || finalCancel) {
      mode = finalCancel ? "cancelled" : r.code === 4 ? "saved" : "uncertain";
      result = finalCancel ? "cancelled" : r.code === 4 ? "saved" : "storage-uncertain";
      pending = null; verified = null; trial = null; phase = r.phase; return true;
    }
    const validCode = action === "status" || action === "stage" ? r.code <= 3 :
      action === "save" && (r.code === 2 || r.code === 3);
    const newTrial = action === "stage" && r.code === 1 && r.phase === 1 && trial === null;
    if (!validCode || (action === "stage" && r.code === 1 && !newTrial) ||
        (r.trial !== null && !newTrial && r.trial !== trial)) {
      stop("unexpected-reply"); return false;
    }
    phase = r.phase; trial = r.trial; pending = null;
    verified = action === "status" && r.code === 1 && phase === 2 ? now : null;
    result = r.code === 2 ? "rejected" : r.code === 3 ? "retry-status" :
      phase === 1 ? "trial" : phase === 2 ? "verified" : phase === 3 ? "draining" : "ready";
    return true;
  }
  function disconnected() { stop("disconnected"); }
  return Object.freeze({ authenticated, snapshot, begin, receive, disconnected });
}
