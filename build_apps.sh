#!/bin/bash

# client_app と server_app のクリーンアップ
make -C client clean
make -C server clean
echo "✅ client_app and server_app cleaned."

# client_app のビルド
make -C client
echo "✅ client_app compiled."

# server_app のビルド
make -C server
echo "✅ server_app compiled."