#!/bin/bash

echo "=== Fixing SSH Repository Access ==="

echo "Current SSH test result shows key is linked to 'voicetotextninja' repository."
echo "We need to either:"
echo "1. Add the SSH key to your main GitHub account, OR"
echo "2. Use HTTPS authentication instead"
echo ""

echo "=== SOLUTION 1: Add SSH Key to Main Account (Recommended) ==="
echo ""
echo "Your SSH key is currently set as a deploy key for 'voicetotextninja'."
echo "To use it for multiple repositories, add it to your main GitHub account:"
echo ""
echo "1. Copy your public key:"
echo "   cat ~/.ssh/sparse_id.pub"
echo ""
echo "2. Go to your GitHub account settings:"
echo "   https://github.com/settings/keys"
echo ""
echo "3. Click 'New SSH key'"
echo "4. Title: 'Mac M1 Development Key'"
echo "5. Paste the key content"
echo "6. Click 'Add SSH key'"
echo ""
echo "7. Remove the old deploy key from voicetotextninja (if needed):"
echo "   https://github.com/shepherdvovkes/voicetotextninja/settings/keys"
echo ""

echo "=== SOLUTION 2: Use HTTPS (Quick Fix) ==="
echo ""

# Switch to HTTPS
echo "Switching to HTTPS remote..."
git remote set-url origin https://github.com/shepherdvovkes/sparse-matrix-multiplication-benchmark.git

echo "Remote updated to:"
git remote -v

echo ""
echo "Now let's check if the repository exists and try to push..."

# Check if repository exists
if curl -s -f https://api.github.com/repos/shepherdvovkes/sparse-matrix-multiplication-benchmark >/dev/null 2>&1; then
    echo "✓ Repository exists on GitHub"
    
    echo ""
    echo "Attempting HTTPS push..."
    echo "When prompted, use:"
    echo "  Username: shepherdvovkes"
    echo "  Password: [Your Personal Access Token]"
    echo ""
    echo "If you don't have a token, create one at: https://github.com/settings/tokens"
    echo "Required scopes: 'repo'"
    echo ""
    
    if git push -u origin main; then
        echo ""
        echo "🎉 SUCCESS! Repository uploaded successfully!"
        echo ""
        echo "Repository URL: https://github.com/shepherdvovkes/sparse-matrix-multiplication-benchmark"
        echo ""
    else
        echo ""
        echo "Push failed. Please check:"
        echo "1. Repository exists: https://github.com/shepherdvovkes/sparse-matrix-multiplication-benchmark"
        echo "2. You have the correct Personal Access Token"
        echo "3. Token has 'repo' permissions"
    fi
    
else
    echo "❌ Repository doesn't exist yet."
    echo ""
    echo "Please create it manually:"
    echo "1. Go to: https://github.com/new"
    echo "2. Repository name: sparse-matrix-multiplication-benchmark"
    echo "3. Description: High-performance sparse matrix multiplication using TCSC format with C optimizations for Apple Silicon CPUs"
    echo "4. Public repository"
    echo "5. Do NOT initialize with README"
    echo "6. Click 'Create repository'"
    echo ""
    echo "Then run this script again or just run:"
    echo "  git push -u origin main"
fi

echo ""
echo "=== Current Repository Status ==="
git status
echo ""
echo "Files ready to upload:"
find . -maxdepth 1 -type f -name "*.cpp" -o -name "*.h" -o -name "*.sh" -o -name "*.txt" | head -10
echo "... and $(find . -type f -not -path './.git/*' | wc -l) total files"

echo ""
echo "=== Your SSH Public Key (for GitHub account setup) ==="
cat ~/.ssh/sparse_id.pub