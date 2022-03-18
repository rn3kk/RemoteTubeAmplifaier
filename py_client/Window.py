import wx
import wx.lib.agw.knobctrl as KC
from wx import CheckBox

from py_client.PA_Client import PA_Client


class Window(wx.Frame):
    mech1 = None

    def __init__(self, parent):
        wx.Frame.__init__(self, parent, -1, "Тестовое окно")
        self.SetSize((270, 270))

        s1 = wx.BoxSizer(wx.VERTICAL)
        l = wx.StaticText(self, wx.NewId(), label="Tune", size=(80, -1), style=wx.TE_CENTER)
        self.__knob1 = KC.KnobCtrl(self, wx.NewId(), size=(80, 80))
        self.__knob1._maxvalue = 359
        self.__knob1.SetAngularRange(0, 359)
        self.__knob1.SetValue(0)
        self.__check1 = CheckBox(self, wx.NewId(), label='Manual')
        self.__check1.Bind(wx.EVT_CHECKBOX, self.__onManualCheched)
        self.__angle1 = wx.TextCtrl(self, wx.NewId(), value="0", size=(80, -1), style=wx.TE_CENTER)
        self.__knob1.Bind(wx.EVT_MOUSEWHEEL, self.onMouseWheel)
        s1.Add(l)
        s1.Add(self.__knob1)
        s1.Add(self.__angle1)
        s1.Add(self.__check1)

        s2 = wx.BoxSizer(wx.VERTICAL)
        l = wx.StaticText(self, wx.NewId(), label="Load", size=(80, -1), style=wx.TE_CENTER)
        self.__knob2 = KC.KnobCtrl(self, wx.NewId(), size=(80, 80))
        self.__knob2._maxvalue = 359
        self.__knob2.SetAngularRange(0, 359)
        self.__knob2.SetValue(0)
        self.__check2 = CheckBox(self, wx.NewId(), label='Manual')
        self.__check2.Bind(wx.EVT_CHECKBOX, self.__onManualCheched)
        self.__angle2 = wx.TextCtrl(self, wx.NewId(), value="0", size=(80, -1), style=wx.TE_CENTER)
        self.__knob2.Bind(wx.EVT_MOUSEWHEEL, self.onMouseWheel)
        s2.Add(l)
        s2.Add(self.__knob2)
        s2.Add(self.__angle2)
        s2.Add(self.__check2)

        main_sizer = wx.BoxSizer(wx.HORIZONTAL)
        main_sizer.Add(s1)
        main_sizer.Add(s2)
        self.SetSizer(main_sizer)
        self.Layout()

        self.__radio_client = PA_Client()
        self.__radio_client.start()

        # self.settings = Settings.getInstance()
        # self.mech1 = Mechaduino(self.settings.mech1_name, self.settings.mech1_port, self.mech1)
        # self.mech1.start()

    def onMouseWheel(self, event):
        o = event.GetEventObject()
        if o == self.__knob1:
            if event.GetWheelRotation() > 0:
                if self.__knob1.GetValue() + 1 > 359:
                    self.__knob1.SetValue(0)
                else:
                    self.__knob1.SetValue(self.__knob1.GetValue() + 1)
            else:
                if self.__knob1.GetValue() - 1 < 0:
                    self.__knob1.SetValue(359)
                else:
                    self.__knob1.SetValue(self.__knob1.GetValue() - 1)
            self.__angle1.SetValue(str(self.__knob1.GetValue()))
        elif o == self.__knob2:
            if event.GetWheelRotation() > 0:
                if self.__knob2.GetValue() + 1 > 359:
                    self.__knob2.SetValue(0)
                else:
                    self.__knob2.SetValue(self.__knob2.GetValue() + 1)
            else:
                if self.__knob2.GetValue() - 1 < 0:
                    self.__knob2.SetValue(359)
                else:
                    self.__knob2.SetValue(self.__knob2.GetValue() - 1)
            self.__angle2.SetValue(str(self.__knob2.GetValue()))

    def __onManualCheched(self, event):
        print('click')
        o = event.GetEventObject()
        if o == self.__check1:
            self.__check1.SetValue(False)
        elif o == self.__check2:
            self.__check2.SetValue(False)
