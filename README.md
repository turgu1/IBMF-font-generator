## IBMF Tailored font generator

This is a tool to generate a tailored IBMF font for a single EPub ebook. The font is expected to be integrated inside an EPub compressed file.

The tool retrieves all character code points present in the book and extracts the character glyphs from the GNU Unifont hex file. The generated font is named `font.ibmf`.