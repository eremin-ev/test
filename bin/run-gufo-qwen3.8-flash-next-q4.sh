#!/bin/bash
podman run --rm --userns=keep-id:uid=1000,gid=100 --group-add keep-groups \
           --device /dev/kfd --device /dev/dri --ulimit memlock=-1 \
           --publish 127.0.0.1:18080:18080 \
           --log-driver=none \
           -v ~/models:/models:ro ghcr.io/gufo-org/toolboxes/gufo-runtime:latest \
           gufo serve --host 0.0.0.0 --port 18080 llm \
                      --model '/models/qwen3.8-flash-next/UD-Q4_K_XL/Qwen3.8-Flash-Next-UD-Q4_K_XL-00001-of-00004.gguf' \
                      --speculative mtp \
                      --mtp-model '/models/qwen3.8-flash-next/MTP/mtp-Qwen3.8-Flash-Next-shared-Q8_0.gguf'
