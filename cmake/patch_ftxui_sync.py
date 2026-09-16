import sys

def patch_ftxui(file_path):
    with open(file_path, 'r') as f:
        content = f.read()
    
    content = content.replace(
        'TerminalSend("\\033[?25l");',
        'TerminalSend("\\033[?2026h\\033[?25l");'
    )
    
    content = content.replace(
        'TerminalSend(set_cursor_position_);\n  TerminalFlush();',
        'TerminalSend(set_cursor_position_);\n  TerminalSend("\\033[?2026l");\n  TerminalFlush();'
    )
    
    with open(file_path, 'w') as f:
        f.write(content)

if __name__ == '__main__':
    if len(sys.argv) > 1:
        patch_ftxui(sys.argv[1])