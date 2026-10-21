#!/bin/bash
set -e

echo "=== GitHub Push Script for spc-10000 ==="
echo "Target remote: git@github.com:shimmany-devel/spc-10000.git"
echo "Branch: main"
echo ""

git push -u origin main

echo ""
echo "=== Push completed successfully! ==="
