#!/bin/bash

# client_app のビルド
gcc client/src/client_app.c -o client_app.out
echo "✅ client_app compiled."

# server_app のビルド
gcc server/src/server_app.c -o server_app.out
echo "✅ server_app compiled."
