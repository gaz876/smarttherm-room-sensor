import { createSetupView, decodeSetupReply } from "./view_model.mjs";
function runSetupViewTests(createSetupView, decodeSetupReply) {
  let passed = 0;
  function check(value, name) { if (!value) throw new Error(name); passed++; }
  function reply(code, phase, id = 0) {
    const b = new Uint8Array(11); b[0] = 1; b[1] = code; b[2] = phase; b[10] = id; return b;
  }
  function ready() {
    const v = createSetupView(); if (!v.authenticated(0)) throw new Error("fixture auth");
    const stage = v.begin("stage", 1);
    if (!v.receive(stage, reply(1, 1, 1), 2)) throw new Error("fixture stage");
    const status = v.begin("status", 3);
    if (!v.receive(status, reply(1, 2, 1), 4)) throw new Error("fixture status");
    return v;
  }
  for (let code = 0; code <= 7; code++) for (let phase = 0; phase <= 4; phase++) {
    for (const id of [0, 1]) {
      const valid = code >= 1 && code <= 6 && phase <= 3 && ((phase === 1 || phase === 2) === (id !== 0));
      check(Boolean(decodeSetupReply(reply(code, phase, id))) === valid, "decoder matrix");
    }
  }
  check(decodeSetupReply(null) === null, "null reply");
  check(decodeSetupReply(new Uint8Array(10)) === null, "short reply");
  check(decodeSetupReply(new Uint8Array(12)) === null, "long reply");
  const wrong = reply(1, 0); wrong[0] = 2;
  check(decodeSetupReply(wrong) === null, "version");
  const max = reply(1, 2); max.fill(255, 3);
  check(decodeSetupReply(max).trial === "ffffffffffffffff", "exact 64-bit token");
  let v = createSetupView();
  check(v.begin("stage", 0) === null && !v.snapshot(0).canSave, "locked");
  check(v.authenticated(0) && !v.authenticated(1), "one auth");
  let id = v.begin("stage", 2);
  check(id !== null && v.begin("status", 2) === null, "one outstanding");
  check(!v.receive(id + 1, reply(1, 1, 1), 3) && v.snapshot(3).busy, "stale callback");
  check(v.receive(id, reply(1, 1, 1), 4) && !v.snapshot(4).canSave, "trial not saved");
  id = v.begin("status", 5);
  check(v.receive(id, reply(1, 2, 1), 6) && v.snapshot(6).canSave, "verified");
  check(v.begin("save", 7) === null && v.begin("save", 7, 1) === null, "explicit confirmation");
  id = v.begin("save", 7, true);
  check(v.receive(id, reply(4, 3), 8) && v.snapshot(8).mode === "saved", "save receipt");
  check(v.begin("save", 9, true) === null && !v.authenticated(9), "saved terminal");
  v = ready();
  check(v.snapshot(2003).canSave && !v.snapshot(2004).canSave, "fresh status boundary");
  check(v.begin("save", 2004, true) === null, "stale status no save");
  id = v.begin("status", 2005);
  check(v.receive(id, reply(1, 2, 2), 2006) === false && v.snapshot(2006).mode === "closed", "wrong trial");
  v = ready(); id = v.begin("save", 5, true); v.disconnected();
  check(v.snapshot(6).mode === "uncertain" && v.begin("save", 6, true) === null, "lost save no retry");
  v = ready(); id = v.begin("save", 5, true);
  check(!v.receive(id, reply(4, 3), 2005) && v.snapshot(2005).mode === "uncertain", "late save");
  v = ready(); id = v.begin("save", 5, true);
  check(v.receive(id, reply(5, 3), 6) && v.snapshot(6).mode === "uncertain", "storage uncertain");
  v = ready(); id = v.begin("save", 5, true);
  check(v.receive(id, reply(2, 2, 1), 6) && !v.snapshot(6).canSave, "rejected needs fresh status");
  id = v.begin("status", 7);
  check(v.receive(id, reply(3, 2, 1), 8) && !v.snapshot(8).canSave, "wait not verified");
  id = v.begin("status", 9);
  check(v.receive(id, reply(1, 3), 10) && !v.snapshot(10).canStage, "draining");
  id = v.begin("status", 11);
  check(v.receive(id, reply(1, 0), 12) && v.snapshot(12).canStage, "drained");
  v = ready(); id = v.begin("cancel", 5);
  check(v.receive(id, reply(6, 3), 6) && v.snapshot(6).mode === "cancelled", "cancel receipt");
  v = createSetupView(); v.authenticated(0); id = v.begin("cancel", 1);
  check(v.receive(id, reply(6, 0), 2) && v.snapshot(2).mode === "cancelled", "idle cancel");
  v = ready(); id = v.begin("status", 5);
  check(!v.receive(id, reply(4, 3), 6) && v.snapshot(6).mode === "closed", "no false save");
  v = ready(); id = v.begin("save", 5, true);
  check(!v.receive(id, reply(4, 0), 6) && v.snapshot(6).mode === "uncertain", "invalid save receipt");
  v = ready(); id = v.begin("save", 5, true);
  check(!v.receive(id, null, 6) && v.snapshot(6).mode === "uncertain", "bad save reply");
  v = ready(); check(v.snapshot(300000).mode === "closed", "window expiry");
  v = ready(); check(v.snapshot(3).mode === "closed", "clock reversal");
  v = ready(); check(v.snapshot(NaN).mode === "closed", "invalid time");
  v = ready(); id = v.begin("status", 5);
  check(!v.receive(id, reply(1, 2, 1), 2005) && v.snapshot(2005).mode === "closed", "late status");
  v = createSetupView(); v.authenticated(0); id = v.begin("stage", 1);
  check(!v.receive(id, reply(1, 2, 1), 2) && v.snapshot(2).mode === "closed", "unexpected stage reply");
  return passed;
}
console.log(`browser setup model: ${runSetupViewTests(createSetupView, decodeSetupReply)} assertions PASS`);
