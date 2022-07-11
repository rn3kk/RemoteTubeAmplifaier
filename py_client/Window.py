import wx
import wx.lib.agw.knobctrl as KC
from wx import CheckBox

from py_client.PA_Client import PA_Client


class Window(wx.Frame):
    # mech1 = None
    __relay_list = ['0', '1', '2', '3', '4', '5', '6', '7', '8']
    __pa_client = None

    def __init__(self, parent):
        wx.Frame.__init__(self, parent, -1, "Bi4PowerAmplifier")
        self.SetSize((500, 270))
        self.Bind(wx.EVT_CLOSE, self.OnClose)

        f = wx.BoxSizer(wx.VERTICAL)
        self.__pwr_btn = wx.Button(self, wx.NewId(), label="PWR", size=(80, -1))
        self.__radio_found = wx.StaticText(self, wx.NewId(), label="No Radio", size=(80, -1), style=wx.TE_CENTER)
        self.__reset_protection_btn = wx.Button(self, wx.NewId(), label="Reset", size=(80, -1))
        self.__edit_mode_checkbox = CheckBox(self, wx.NewId(), label='Edit')
        self.__bandpath_checkbox = CheckBox(self, wx.NewId(), label='Bandpass')
        f.Add(self.__pwr_btn)
        f.Add(self.__radio_found)
        f.Add(self.__reset_protection_btn)
        f.Add(self.__edit_mode_checkbox)
        f.Add(self.__bandpath_checkbox)
        self.__edit_mode_checkbox.Bind(wx.EVT_CHECKBOX, self.__edit_mode_ckeckbox_click)
        self.__bandpath_checkbox.Bind(wx.EVT_CHECKBOX, self.__bandpath_click)

        self.__pwr_btn.Bind(wx.EVT_BUTTON, self.__pwr_btn_click)
        self.__reset_protection_btn.Bind(wx.EVT_BUTTON, self.__reset_protection_btn_click)

        s1 = wx.BoxSizer(wx.VERTICAL)
        l = wx.StaticText(self, wx.NewId(), label="Tune", size=(80, -1), style=wx.TE_CENTER)
        self.__knob1 = KC.KnobCtrl(self, wx.NewId(), size=(80, 80))
        self.__knob1._maxvalue = 359
        self.__knob1.SetAngularRange(0, 359)
        self.__knob1.SetValue(0)
        self.__check1 = CheckBox(self, wx.NewId(), label='Manual')
        self.__check1.Bind(wx.EVT_CHECKBOX, self.__onManualCheched)
        self.__angle1 = wx.TextCtrl(self, wx.NewId(), value="-1", size=(80, -1), style=wx.TE_CENTER)
        self.__knob1.Bind(wx.EVT_MOUSEWHEEL, self.onMouseWheel)
        self.__relay_label = wx.StaticText(self, wx.NewId(), label="Relay ->", size=(80, -1), style=wx.TE_CENTER)
        s1.Add(l)
        s1.Add(self.__knob1)
        s1.Add(self.__angle1)
        s1.Add(self.__check1)
        s1.Add(self.__relay_label, 0, wx.EXPAND | wx.TOP, 7)
        self.__s1 = s1

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
        self.__relay_combox = wx.ComboBox(self, wx.NewId(), value='0', choices=self.__relay_list, size=(70, -1))
        self.__relay_combox.Bind(wx.EVT_COMBOBOX, self.__relay_changed_event)
        s2.Add(l)
        s2.Add(self.__knob2)
        s2.Add(self.__angle2)
        s2.Add(self.__check2)
        s2.Add(self.__relay_combox)

        freq_box  = wx.BoxSizer(wx.HORIZONTAL)
        self.__down_freq = wx.Button(self, wx.NewId(), label="<", size=(20, -1))
        self.__up_freq = wx.Button(self, wx.NewId(), label=">", size=(20, -1))
        self.__freq_radio = wx.StaticText(self, wx.NewId(), label="freq 14.150.00", size=(110, -1), style=wx.TE_CENTER)
        self.__down_freq.Bind(wx.EVT_BUTTON, self.__freq_down)
        self.__up_freq.Bind(wx.EVT_BUTTON, self.__freq_up)
        freq_box.Add(self.__down_freq)
        freq_box.Add(self.__freq_radio)
        freq_box.Add(self.__up_freq)
        m = wx.BoxSizer(wx.VERTICAL)
        self.__ptt_state_label = wx.StaticText(self, wx.NewId(), label="none", size=(150, -1), style=wx.TE_CENTER)
        m.Add(freq_box)
        m.Add(self.__ptt_state_label)

        main_sizer = wx.BoxSizer(wx.HORIZONTAL)
        main_sizer.Add(f)
        main_sizer.Add(s1)
        main_sizer.Add(s2)
        main_sizer.Add(m)
        self.SetSizer(main_sizer)

        self.timer = wx.Timer(self)
        self.Bind(wx.EVT_TIMER, self.timerEvent, self.timer)
        self.timer.Start(100)
        self.Layout()

        self.__pa_client = PA_Client()
        self.__pa_client.start()

    def onMouseWheel(self, event):
        o = event.GetEventObject()
        if o == self.__knob1:
            if not self.__check1.GetValue():
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
                self.__pa_client.set_mech1_angle(self.__knob1.GetValue())
        elif o == self.__knob2:
            if not self.__check2.GetValue():
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
        o = event.GetEventObject()
        if o == self.__check1:
            self.__check1.SetValue(False)
            self.__pa_client.mech1_manual()
        elif o == self.__check2:
            self.__check2.SetValue(False)
            self.__pa_client.mech1_manual()

    def __relay_changed_event(self, event):
        v = self.__relay_combox.GetValue()
        self.__pa_client.set_relay(v)

    def __pwr_btn_click(self, event):
        self.__pa_client.change_pwr()

    def __reset_protection_btn_click(self, event):
        self.__pa_client.reset_protection()

    def timerEvent(self, event):
        pwr, ptt, m1_angle, m1_manual_mode, relay_num, edit_mode, trx_found, trx_freq, bandpath = self.__pa_client.get_state()
        if bandpath:
            self.__bandpath_checkbox.SetValue(1)
        else:
            self.__bandpath_checkbox.SetValue(0)
        if trx_found:
            self.__radio_found.SetLabel('Radio')
            self.__radio_found.SetBackgroundColour(wx.GREEN)
            self.__freq_radio.SetLabel(f'Freq:{trx_freq}')
        else:
            self.__radio_found.SetLabel('No radio')
            self.__radio_found.SetBackgroundColour(wx.NullColour)
            self.__freq_radio.SetLabel('0')
        if pwr == 1:
            self.__pwr_btn.SetLabel("On")
        else:
            self.__pwr_btn.SetLabel("Off")

        if ptt == 1:
            self.__ptt_state_label.SetLabel('TX')
            self.__ptt_state_label.SetBackgroundColour((220,20,60))
        else:
            self.__ptt_state_label.SetLabel('RX')
            self.__ptt_state_label.SetBackgroundColour((127, 255, 212))

        self.__check1.SetValue(m1_manual_mode)

        if m1_manual_mode == 1:
            self.__knob1.Enable(False)
            self.__knob1.SetValue(m1_angle)
        else:
            self.__knob1.Enable(True)
            if self.__angle1.GetValue() == '-1':
                self.__knob1.SetValue(m1_angle)

        self.__angle1.SetValue(str(m1_angle))
        self.__relay_combox.SetValue(str(relay_num))

        self.__edit_mode_changed(edit_mode)

    def __edit_mode_ckeckbox_click(self, event):
        self.__edit_mode_checkbox.SetValue(0)
        self.__pa_client.change_edit_mode()

    def __bandpath_click(self, event):
        self.__bandpath_checkbox.SetValue(0)
        self.__pa_client.change_bandpath()

    def OnClose(self, event):
        self.timer.Stop()

        if self.__pa_client:
            self.__pa_client.set_terminate()
            self.__pa_client.join()
        self.Destroy()

    def __edit_mode_changed(self, value):
            b = bool(value)
            self.__edit_mode_checkbox.SetValue(b)
            self.__knob1.Enable(b)
            self.__knob2.Enable(b)
            self.__check1.Enable(b)
            self.__check2.Enable(b)
            self.__angle1.Enable(b)
            self.__angle2.Enable(b)
            self.__relay_label.Enable(b)
            self.__relay_combox.Enable(b)

    def __freq_down(self, event):
        self.__pa_client.change_freq_to_down()

    def __freq_up(self, event):
        self.__pa_client.change_freq_to_up()

