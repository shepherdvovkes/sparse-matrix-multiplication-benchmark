#!/bin/bash

echo "=== Quick HTTPS Push Solution ==="

# Navigate to project directory
cd /Users/vovkes/Downloads/team38-papi-branch

echo "Current git status:"
git status

echo ""
echo "Current remotes:"
git remote -v

echo ""
echo "=== Switching to HTTPS remote ==="
git remote set-url origin https://github.com/shepherdvovkes/sparse-matrix-multiplication-benchmark.git

echo "Updated remote:"
git remote -v

echo ""
echo "=== Creating repository on GitHub (if needed) ==="
echo "Please manually create the repository:"
echo "1. Go to: https://github.com/new"
echo "2. Repository name: sparse-matrix-multiplication-benchmark"
echo "3. Description: High-performance sparse matrix multiplication using TCSC format with C optimizations for Apple Silicon CPUs"
echo "4. Public repository"
echo "5. Do NOT initialize with README"
echo "6. Click 'Create repository'"
echo ""

read -p "Have you created the repository on GitHub? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "=== Attempting to push via HTTPS ==="
    echo "You will be prompted for GitHub username and password/token."
    echo "Note: Use your GitHub username and a Personal Access Token as password."
    echo "Create token at: https://github.com/settings/tokens"
    echo ""
    
    # Show what we're about to push
    echo "Commits to push:"
    git log --oneline origin/main..HEAD 2>/dev/null || git log --oneline -5
    
    echo ""
    echo "Pushing to GitHub..."
    if git push -u origin main; then
        echo ""
        echo "*** SUCCESS! Repository uploaded! ***"
        echo ""
        echo "🎉 Your repository is now live at:"
        echo "   https://github.com/shepherdvovkes/sparse-matrix-multiplication-benchmark"
        echo ""
        echo "📋 Next steps:"
        echo "1. Visit your repository and verify all files are there"
        echo "2. Add repository topics: sparse-matrix, linear-algebra, performance-optimization, apple-silicon"
        echo "3. Star your repository to increase visibility"
        echo "4. Share with colleagues and the HPC community!"
        echo ""
    else
        echo ""
        echo "Push failed. Common solutions:"
        echo "1. Make sure repository exists on GitHub"
        echo "2. Use GitHub username (not email) when prompted"
        echo "3. Use Personal Access Token as password (not GitHub password)"
        echo "4. Token needs 'repo' permissions"
        echo ""
        echo "Create token at: https://github.com/settings/tokens"
    fi
else
    echo ""
    echo "Please create the repository first, then run this script again."
    echo "Repository URL: https://github.com/new"
fi

echo ""
echo "=== Repository Information ==="
echo "Local path: $(pwd)"
echo "Files ready: $(find . -type f -not -path './.git/*' | wc -l) files"
echo "Benchmark executable: $(ls -la tcsc_benchmark 2>/dev/null && echo 'Ready!' || echo 'Missing')"
echo "Latest results: $(ls -la out.txt 2>/dev/null && echo 'Available' || echo 'Not found')"