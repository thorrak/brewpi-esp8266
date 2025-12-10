#!/bin/bash
# Script to create and push the v16_new_ow branch
# This script recreates the branch creation process

set -e  # Exit on error

echo "Creating v16_new_ow branch..."

# Ensure we have all the latest branches
echo "Fetching latest branches..."
git fetch origin

# Checkout v16 as the base
echo "Checking out v16 branch..."
git checkout v16 2>/dev/null || git checkout -b v16 origin/v16

# Create the new branch
echo "Creating v16_new_ow from v16..."
git checkout -b v16_new_ow 2>/dev/null || git checkout v16_new_ow

# Reset to v16 to ensure clean state
git reset --hard origin/v16

# Merge esp_ow_lib
echo "Merging esp_ow_lib branch..."
git merge origin/esp_ow_lib --no-edit

# Update build.yml
echo "Updating build.yml..."
if ! grep -q "v16_new_ow" .github/workflows/build.yml; then
    # Use sed to add v16_new_ow after v16 line
    sed -i '/^\s*- v16$/a\      - v16_new_ow' .github/workflows/build.yml
    git add .github/workflows/build.yml
    git commit -m "Add v16_new_ow branch to build.yml workflow"
    echo "build.yml updated and committed"
else
    echo "build.yml already contains v16_new_ow"
fi

# Show the result
echo ""
echo "Branch v16_new_ow created successfully!"
echo ""
echo "Commit log:"
git log --oneline --graph -5

echo ""
echo "To push this branch to the remote, run:"
echo "  git push -u origin v16_new_ow"
