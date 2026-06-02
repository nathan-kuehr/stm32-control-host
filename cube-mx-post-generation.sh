#!/usr/bin/env zsh
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
pre-commit run --all-files || true
