#!/bin/bash
# C-Express Documentation Builder
# Generates API documentation using Doxygen

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}  C-Express Documentation Builder${NC}"
echo -e "${BLUE}================================================${NC}"
echo ""

# Check if Doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo -e "${RED}Error: Doxygen is not installed${NC}"
    echo ""
    echo "Please install Doxygen:"
    echo "  Ubuntu/Debian: sudo apt install doxygen"
    echo "  Fedora/RHEL:   sudo dnf install doxygen"
    echo "  macOS:         brew install doxygen"
    echo ""
    exit 1
fi

DOXYGEN_VERSION=$(doxygen --version)
echo -e "${GREEN}✓${NC} Doxygen found (version $DOXYGEN_VERSION)"

# Check if Doxyfile exists
if [ ! -f "Doxyfile" ]; then
    echo -e "${RED}Error: Doxyfile not found${NC}"
    echo "Please run this script from the project root directory"
    exit 1
fi

echo -e "${GREEN}✓${NC} Doxyfile found"

# Clean old documentation
if [ -d "docs" ]; then
    echo -e "${YELLOW}→${NC} Cleaning old documentation..."
    rm -rf docs/*
fi

# Create docs directory if it doesn't exist
mkdir -p docs

# Generate documentation
echo -e "${YELLOW}→${NC} Generating documentation..."
echo ""

if doxygen Doxyfile; then
    echo ""
    echo -e "${GREEN}================================================${NC}"
    echo -e "${GREEN}  Documentation generated successfully!${NC}"
    echo -e "${GREEN}================================================${NC}"
    echo ""
    
    # Count generated files
    HTML_COUNT=$(find docs -name "*.html" 2>/dev/null | wc -l)
    TOTAL_SIZE=$(du -sh docs 2>/dev/null | cut -f1)
    
    echo -e "  Location: ${BLUE}docs/${NC}"
    echo -e "  Main file: ${BLUE}docs/index.html${NC}"
    echo -e "  HTML files: ${GREEN}$HTML_COUNT${NC}"
    echo -e "  Total size: ${GREEN}$TOTAL_SIZE${NC}"
    echo ""
    
    # Ask to open in browser
    read -p "Do you want to open the documentation in your browser? [y/N] " -n 1 -r
    echo ""
    
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        INDEX_FILE="$SCRIPT_DIR/docs/index.html"
        
        # Try to open with appropriate command based on OS
        if command -v xdg-open &> /dev/null; then
            xdg-open "$INDEX_FILE" &> /dev/null &
        elif command -v open &> /dev/null; then
            open "$INDEX_FILE"
        elif command -v start &> /dev/null; then
            start "$INDEX_FILE"
        else
            echo -e "${YELLOW}Could not detect browser command${NC}"
            echo "Please open manually: $INDEX_FILE"
        fi
        
        echo -e "${GREEN}✓${NC} Opening documentation in browser..."
    fi
    
    echo ""
    echo -e "To view later, open: ${BLUE}file://$SCRIPT_DIR/docs/index.html${NC}"
    echo ""
    
    exit 0
else
    echo ""
    echo -e "${RED}================================================${NC}"
    echo -e "${RED}  Documentation generation failed!${NC}"
    echo -e "${RED}================================================${NC}"
    echo ""
    exit 1
fi
