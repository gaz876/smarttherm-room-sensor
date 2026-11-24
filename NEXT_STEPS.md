# Next steps
Implement and test authenticated freshness across restarts, reserve setpoint and PID recovery. Audit CI separately before enabling it. Hardware integration requires bench validation.
Host test: `g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=undefined -Isrc tests/test.cpp -o /tmp/safetytest && /tmp/safetytest`.
