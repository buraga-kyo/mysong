"""Keep each frame atomic and change cursor modes only when needed."""
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
content = path.read_text()
content = content.replace('TerminalSend("\\033[?25l");', 'TerminalSend("\\033[?2026h");')
content = content.replace('TerminalSend("\\033[?2026h\\033[?25l");', 'TerminalSend("\\033[?2026h");')
content = content.replace(
    'TerminalSend(set_cursor_position_);\n  TerminalFlush();',
    'TerminalSend(set_cursor_position_);\n  TerminalSend("\\033[?2026l");\n  TerminalFlush();')
if 'mysong_cursor_shape_' not in content:
    content = content.replace('  int cursor_reset_shape_ = 1;',
        '  int cursor_reset_shape_ = 1;\n  int mysong_cursor_shape_ = -1;')
    content = content.replace('      set_cursor_position_ += "\\033[?25h";\n      set_cursor_position_ +=\n          "\\033[" + std::to_string(int(public_->cursor_.shape)) + " q";', '')
    anchor = '  public_->ToString(output_buffer);'
    if anchor not in content:
        raise SystemExit('FTXUI: frame drawing anchor missing')
    content = content.replace(anchor, '''  const int cursor_shape = int(public_->cursor_.shape);
  if (mysong_cursor_shape_ != cursor_shape) {
    set_cursor_position_ += cursor_shape == 0 ? "\\033[?25l" : "\\033[?25h";
    if (cursor_shape != 0)
      set_cursor_position_ += "\\033[" + std::to_string(cursor_shape) + " q";
    mysong_cursor_shape_ = cursor_shape;
  }
''' + anchor)
    content = content.replace('void App::Internal::Install() {',
        'void App::Internal::Install() {\n  mysong_cursor_shape_ = -1;')
# Resize must be distinguishable from Event::Custom (both were Special({0})).
content = content.replace('public_->Post(Event::Special({0}));',
    'public_->Post(Event::Special("mysong:resize"));')
if 'mysong_cursor_shape_' not in content or '\\033[?2026l' not in content:
    raise SystemExit('FTXUI: cursor/sync patch incomplete')
if content != path.read_text():
    path.write_text(content)
