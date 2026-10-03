#!/bin/bash
podman run --rm --userns=keep-id:uid=1000,gid=100 --group-add keep-groups \
           --device /dev/kfd --device /dev/dri --ulimit memlock=-1 \
           --publish 127.0.0.1:18080:18080 \
           --log-driver=none \
           -v ~/models:/models:ro ghcr.io/gufo-org/toolboxes/gufo-runtime:latest \
           gufo serve --host 0.0.0.0 --port 18080 llm \
                      --model '/models/qwen3.8-27b/Qwen3.8-27B-UD-Q8_K_XL.gguf' \
                      --speculative dflash2 \
                      --dflash-model '/models/qwen3.8-27b/Qwen3.8-27B-DFlash2-Q4_K_M.gguf'
