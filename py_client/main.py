import wx

from py_client.Window import Window

def main():
    app = wx.App(0)
    frame = Window(None)
    app.SetTopWindow(frame)
    frame.Show()
    app.MainLoop()

if __name__ == '__main__':
    main()
    exit(0)