import wx
import wx.lib.agw.knobctrl as KC
from wx import CheckBox

from py_server.Mechaduino import Mechaduino
from py_server.settings import Settings


class Window(wx.Frame):
    mech1 = None

    def __init__(self, parent):
        wx.Frame.__init__(self, parent, -1, "Тестовое окно")
        self.SetSize((270, 270))
        s = wx.BoxSizer(wx.HORIZONTAL)
        self.knob1 = KC.KnobCtrl(self, wx.NewId(), size=(58, 58))
        self.knob1._maxvalue = 359
        self.knob1.SetAngularRange(0, 359)
        self.knob1.SetValue(0)
        self.check = CheckBox(self, label='Manual')
        self.check.Bind(wx.EVT_CHECKBOX, self.__onCheched)

        self.knob1.Bind(wx.EVT_MOUSEWHEEL, self.onMouseWheel)
        s.Add(self.knob1)
        s.Add(self.check)

        self.SetSizer(s)
        self.Layout()

        self.settings = Settings.getInstance()
        self.mech1 = Mechaduino(self.settings.mech1_name, self.settings.mech1_port, self.mech1)
        self.mech1.start()


    def onMouseWheel(self, event):
        if event.GetWheelRotation() > 0:
            if self.knob1.GetValue() + 1 > 359:
                self.knob1.SetValue(0)
            else:
                self.knob1.SetValue(self.knob1.GetValue() + 1)
        else:
            if self.knob1.GetValue() - 1 < 0:
                self.knob1.SetValue(359)
            else:
                self.knob1.SetValue(self.knob1.GetValue() - 1)
        self.mech1.set_angle(self.knob1.GetValue())

    def __onCheched(self, event):
        b = self.check.GetValue()
        self.mech1.set_manual_mode(b)
