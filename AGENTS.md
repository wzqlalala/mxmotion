# MxMotion 协作约定

## 界面与渲染校验

- 默认在后台执行界面、渲染和截图回归。使用 `ctest --test-dir build-desktop -L desktop --output-on-failure` 或对应的 `MxMotion.exe --*-smoke` 入口。
- 界面截图用 `QWidget::grab()`，三维渲染截图用真实 OpenGL 帧缓冲 `grabFramebuffer()`。在测试进程内投递 Qt 事件，避免操作前台鼠标、键盘或窗口焦点。
- 若某项检查依赖真实前台交互，先说明需要在前台验证的具体行为。
