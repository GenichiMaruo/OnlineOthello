#!/bin/bash

# client_app のビルド
make -C client
echo "✅ client_app compiled."

# server_app のビルド
make -C server
echo "✅ server_app compiled."