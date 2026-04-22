#!/bin/bash
# Merge all SoftISP branches into a single working tree

echo "=== SoftISP Branch Merge Script ==="
echo ""

# Create a new branch for the merge
git checkout -b merged-softisp-complete 2>/dev/null || {
    echo "Branch merged-softisp-complete already exists, switching to it..."
    git checkout merged-softisp-complete
}

echo "Current branch: $(git branch --show-current)"
echo ""

# List branches to merge
BRANCHES=("libcamera2" "feature/softisp-onnx-inference")

echo "Branches to merge:"
for branch in "${BRANCHES[@]}"; do
    echo "  - $branch"
done
echo ""

# Merge each branch
for branch in "${BRANCHES[@]}"; do
    if [ "$branch" != "merged-softisp-complete" ]; then
        echo "Merging $branch..."
        git merge "$branch" -m "Merge $branch: SoftISP features" || {
            echo "Conflict detected in merge of $branch"
            echo "Please resolve conflicts manually and continue"
            exit 1
        }
        echo "✓ Merged $branch"
        echo ""
    fi
done

echo "=== Merge Complete ==="
echo ""
echo "Summary of changes:"
git log --oneline -10
echo ""
echo "Files changed:"
git diff --stat HEAD~5
echo ""
echo "Next steps:"
echo "1. Review merged files for conflicts"
echo "2. Run tests: ./build/tools/softisp-test-app"
echo "3. Build: meson compile -C build"
echo "4. Commit the merge: git commit -m 'Merge all SoftISP branches'"
