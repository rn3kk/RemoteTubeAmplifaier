#!/usr/bin/env python3
"""Генератор рисунка img/board-pins.svg — выводы Waveshare ESP32-S3-ETH.

Вид сверху, разъём Ethernet обращён к смотрящему (плата развёрнута на 180°
от официальной распиновки Waveshare): слева контакты 20..1, справа 21..40.
Только стандартная библиотека. Запуск: python3 tools/make-board-pins.py

Перед записью файла данные проверяются assert-ами (40 контактов, номера без
повторов, наши пины на своих местах); при расхождении SVG не пишется.
"""

import os
import sys

OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                   "img", "board-pins.svg")

# ---------------------------------------------------------------- геометрия
W, H = 1180, 1584

BOARD_X = 360
BOARD_W = 460
BOARD_X1 = BOARD_X + BOARD_W
BOARD_Y = 192
BOARD_Y1 = 1288

ROW0 = 242          # центр первого контакта
PITCH = 52          # шаг контактов
N_ROWS = 20

COL_BLOCK_W = 306
COL_LEFT_X = 18
COL_RIGHT_X = W - 18 - COL_BLOCK_W

COL_RELAY = "#c2362b"
COL_INPUT = "#b45309"
COL_SERVO = "#0b7285"
COL_TEXT = "#1c2330"
COL_SUB = "#6e7888"

FONT = "'Arial', 'Helvetica Neue', 'DejaVu Sans', sans-serif"

# ---------------------------------------------------------------- данные
LEFT = [(20, "GPIO33"), (19, "GPIO34"), (18, "GND"), (17, "GPIO35"),
        (16, "GPIO36"), (15, "GPIO37"), (14, "GPIO38"), (13, "GND"),
        (12, "GPIO39"), (11, "GPIO40"), (10, "GPIO41"), (9, "GPIO42"),
        (8, "GND"), (7, "GPIO45"), (6, "GPIO46"), (5, "GPIO47"),
        (4, "GPIO48"), (3, "GND"), (2, "GPIO19"), (1, "GPIO20")]

RIGHT = [(21, "GPIO43"), (22, "GPIO44"), (23, "GND"), (24, "GPIO0"),
         (25, "GPIO1"), (26, "GPIO2"), (27, "GPIO3"), (28, "GND"),
         (29, "GPIO15"), (30, "CHIP_PU"), (31, "GPIO18"), (32, "GPIO16"),
         (33, "GND"), (34, "GPIO17"), (35, "GPIO21"), (36, "3V3"),
         (37, "3V3_EN"), (38, "GND"), (39, "VSYS"), (40, "VBUS")]

# ожидаемые имена наших пинов (для самопроверки)
EXPECTED_NAME = {14: "GPIO38", 12: "GPIO39", 11: "GPIO40", 31: "GPIO18",
                 34: "GPIO17", 22: "GPIO44", 21: "GPIO43", 5: "GPIO47",
                 4: "GPIO48", 3: "GND"}

PSRAM_CONTACTS = (20, 19, 17, 16, 15)

# наши подключения: контакт -> (цвет, строка «IOxx · контакт N», функция)
CONN = {
    14: (COL_RELAY, "IO38 · контакт 14", "реле 1: включение 220 В"),
    12: (COL_RELAY, "IO39 · контакт 12",
         "реле 2: плавный пуск (шунт пускового резистора)"),
    11: (COL_RELAY, "IO40 · контакт 11", "реле 3: тестовый запрос PTT усилителя"),
    31: (COL_RELAY, "IO18 · контакт 31", "реле 4: анодное напряжение"),
    34: (COL_RELAY, "IO17 · контакт 34", "реле 8: разрешение PTT"),
    22: (COL_INPUT, "IO44 · контакт 22", "вход PTT от трансивера"),
    21: (COL_INPUT, "IO43 · контакт 21", "вход «сработала защита»"),
    5: (COL_SERVO, "IO47 · контакт 5", "передача (TXD адаптера)"),
    4: (COL_SERVO, "IO48 · контакт 4", "приём (RXD адаптера)"),
    3: (COL_SERVO, "GND · контакт 3", "земля адаптера"),
}

NOTES = ["реле включаются низким уровнем входа",
         "на IO44 и IO43 — только сухой контакт или оптрон",
         "W5500 и SD-карта уже на плате: IO9–IO14 и IO4–IO7 на гребёнки не выведены",
         "свободны входы реле IN5…IN7"]

LEGEND = [("реле, модуль на 8 реле (IN1…IN8)", COL_RELAY),
          ("входы, сухой контакт на GND", COL_INPUT),
          ("шина сервоприводов (UART)", COL_SERVO)]


# ---------------------------------------------------------------- проверки
def validate():
    allpins = LEFT + RIGHT
    assert len(LEFT) == 20 and len(RIGHT) == 20, "в гребёнке не 20 контактов"
    assert len(allpins) == 40, "всего контактов не 40"
    nums = [c for c, _ in allpins]
    assert sorted(nums) == list(range(1, 41)), "номера контактов не 1..40 без повторов"
    assert len(set(nums)) == 40, "номера контактов повторяются"
    names = {c: n for c, n in allpins}
    for contact, name in EXPECTED_NAME.items():
        assert contact in names, "контакта %d нет в таблице" % contact
        assert names[contact] == name, ("контакт %d: ожидалось %s, в таблице %s"
                                        % (contact, name, names[contact]))
    assert set(CONN) == set(EXPECTED_NAME), "набор наших пинов не совпал с CONN"
    psram_names = sorted(names[c] for c in PSRAM_CONTACTS)
    assert psram_names == ["GPIO33", "GPIO34", "GPIO35", "GPIO36", "GPIO37"], \
        "PSRAM отмечена не на тех контактах: %s" % psram_names
    for contact, (_color, head, _func) in CONN.items():
        assert ("контакт %d" % contact) in head, "неверная подпись пина %d" % contact
    assert [c for c, _ in LEFT] == list(range(20, 0, -1)), "порядок левой гребёнки"
    assert [c for c, _ in RIGHT] == list(range(21, 41)), "порядок правой гребёнки"


# ---------------------------------------------------------------- утилиты
def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def row_y(idx):
    return ROW0 + idx * PITCH


def text_w(s, size):
    """Грубая оценка ширины строки (Arial, кириллица)."""
    wide = sum(1 for ch in s if ch in "ШЩЖМЫЮФwWmM")
    narrow = sum(1 for ch in s if ch in " iljI.,:;'!|")
    return ((len(s) - wide - narrow) * size * 0.545
            + wide * size * 0.86 + narrow * size * 0.3)


def wrap(s, size, limit):
    """Разбить строку на 1-2 строки по границе слов (по возможности перед «(»)."""
    if text_w(s, size) <= limit:
        return [s]
    words = s.split(" ")
    best = None
    for i in range(1, len(words)):
        left, right = " ".join(words[:i]), " ".join(words[i:])
        wl, wr = text_w(left, size), text_w(right, size)
        if wl <= limit and wr <= limit:
            score = max(wl, wr) - (12 if right.startswith("(") else 0)
            if best is None or score < best[0]:
                best = (score, [left, right])
    if best:
        return best[1]
    return [s]


# ---------------------------------------------------------------- вывод
def build():
    out = []
    a = out.append

    a('<?xml version="1.0" encoding="UTF-8"?>')
    a('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" '
      'height="%d" role="img" aria-label="Выводы платы Waveshare ESP32-S3-ETH. '
      'Вид сверху, Ethernet сверху, USB-C снизу. Реле: IO38, IO39, IO40, IO18, IO17. '
      'Входы: IO44, IO43. Сервоприводы: IO47, IO48, GND. IO33-IO37 заняты PSRAM." '
      'class="bp">' % (W, H, W, H))
    a('<style>')
    a("  .bp text { font-family: %s; }" % FONT)
    a("  .bp .title { font-size: 27px; font-weight: 700; fill: %s; }" % COL_TEXT)
    a("  .bp .sub { font-size: 15px; fill: %s; }" % COL_SUB)
    a("  .bp .board { fill: #1f2a26; stroke: #0d1512; stroke-width: 2; }")
    a("  .bp .edge { fill: #2c3a34; stroke: #4b5a52; stroke-width: 1; }")
    a("  .bp .shield { fill: #48525c; stroke: #2c343c; stroke-width: 1.5; }")
    a("  .bp .shieldtxt { font-size: 17px; font-weight: 700; fill: #e7ebf0; }")
    a("  .bp .shieldsml { font-size: 12.5px; fill: #b9c3ce; }")
    a("  .bp .pinno { font-size: 15px; font-weight: 700; }")
    a("  .bp .dimno { font-size: 15px; font-weight: 700; }")
    a("  .bp .dimname { font-size: 17px; fill: #93a0ac; }")
    a("  .bp .psramname { font-size: 17px; fill: #b6c1cc; }")
    a("  .bp .usedname { font-size: 17px; font-weight: 700; fill: #f2f5f8; }")
    a("  .bp .stub { stroke: #c9b46a; stroke-width: 4; stroke-linecap: round; }")
    a("  .bp .lead { stroke: %s; stroke-width: 1.4; fill: none; }" % COL_SUB)
    a("  .bp .blk { fill: #ffffff; }")
    a("  .bp .blkt1 { font-size: 15px; font-weight: 700; }")
    a("  .bp .blkt2 { font-size: 15px; fill: #2b3543; }")
    a("  .bp .psram { fill: url(#hatch); opacity: .5; }")
    a("  .bp .psramtxt { font-size: 17px; font-weight: 700; }")
    a("  .bp .psramsml { font-size: 14.5px; }")
    a("  .bp .note { font-size: 15px; fill: #2b3543; }")
    a("  .bp .notebullet { fill: %s; }" % COL_SUB)
    a("  .bp .eth { fill: #cfd4da; stroke: #6b7684; stroke-width: 1.6; }")
    a("  .bp .ethin { fill: #232a31; stroke: #4a545f; stroke-width: 1; }")
    a("  .bp .ethpin { fill: #e0b64b; }")
    a("  .bp .usb { fill: #cfd4da; stroke: #6b7684; stroke-width: 1.4; }")
    a("  .bp .usbin { fill: #3a434c; }")
    a("  .bp .sepline { stroke: #d7dde4; stroke-width: 1; }")
    a("  .bp .grouplbl { font-size: 14px; font-weight: 700; letter-spacing: .06em; }")
    a('</style>')
    a('<defs>')
    a('  <pattern id="hatch" width="7" height="7" patternTransform="rotate(45)" '
      'patternUnits="userSpaceOnUse"><rect width="7" height="7" fill="#8b95a3"/>'
      '<line x1="0" y1="0" x2="0" y2="7" stroke="#5f6a77" stroke-width="3"/></pattern>')
    a('</defs>')

    a('<rect x="0" y="0" width="%d" height="%d" fill="#f6f7f9"/>' % (W, H))

    # ---- заголовок
    a('<text x="%d" y="36" text-anchor="middle" class="title">'
      'Waveshare ESP32-S3-ETH: выводы для подключения</text>' % (W // 2))
    a('<text x="%d" y="62" text-anchor="middle" class="sub">'
      'Вид сверху, Ethernet-разъём к себе. Номера 1–40 — как на плате, '
      'IOxx — как в прошивке.</text>' % (W // 2))

    # ---- плата
    a('<rect x="%d" y="%d" width="%d" height="%d" rx="10" class="board"/>'
      % (BOARD_X, BOARD_Y, BOARD_W, BOARD_Y1 - BOARD_Y))
    header_h = (N_ROWS - 1) * PITCH + 40
    a('<rect x="%d" y="%d" width="152" height="%d" rx="6" class="edge"/>'
      % (BOARD_X + 6, BOARD_Y + 14, header_h))
    a('<rect x="%d" y="%d" width="152" height="%d" rx="6" class="edge"/>'
      % (BOARD_X1 - 158, BOARD_Y + 14, header_h))

    # PSRAM: две штрихованные полосы (контакты 20,19 и 17,16,15)
    for first, last in ((0, 1), (3, 5)):
        ps_y0 = row_y(first) - PITCH // 2 + 4
        ps_h = row_y(last) - row_y(first) + PITCH - 8
        a('<rect x="%d" y="%d" width="152" height="%d" rx="6" class="psram"/>'
          % (BOARD_X + 6, ps_y0, ps_h))

    # ---- Ethernet сверху
    ex, ew, ey, eh = 486, 232, 94, 98
    a('<rect x="%d" y="%d" width="%d" height="%d" rx="6" class="eth"/>'
      % (ex - 10, ey - 10, ew + 20, eh + 10))
    a('<rect x="%d" y="%d" width="%d" height="%d" rx="3" class="ethin"/>'
      % (ex + 18, ey + 16, ew - 36, 58))
    for i in range(8):
        a('<rect x="%.1f" y="%d" width="7" height="16" rx="1" class="ethpin"/>'
          % (ex + 30 + i * 23, ey + 50))
    a('<rect x="%d" y="%d" width="44" height="12" rx="2" class="eth"/>'
      % (ex + ew // 2 - 22, ey - 12))
    a('<text x="%d" y="%d" class="grouplbl" fill="#2b3543">ETHERNET (RJ45)</text>'
      % (ex + ew + 22, ey + 14))
    a('<text x="%d" y="%d" class="sub">витая пара → роутер,</text>'
      % (ex + ew + 22, ey + 36))
    a('<text x="%d" y="%d" class="sub">адрес по DHCP</text>'
      % (ex + ew + 22, ey + 55))

    # ---- USB-C снизу
    ux, uw, uy, uh = 520, 140, BOARD_Y1 + 6, 44
    a('<rect x="%d" y="%d" width="%d" height="%d" rx="12" class="usb"/>'
      % (ux, uy, uw, uh))
    a('<rect x="%d" y="%d" width="%d" height="%d" rx="7" class="usbin"/>'
      % (ux + 14, uy + 10, uw - 28, uh - 20))
    a('<text x="%d" y="%d" text-anchor="middle" class="grouplbl" fill="#2b3543">'
      'USB-C: прошивка и питание</text>' % (W // 2, uy + uh + 24))

    # ---- центральный модуль
    sh_x, sh_w = 520, 140
    a('<rect x="%d" y="%d" width="%d" height="300" rx="4" class="shield"/>'
      % (sh_x, ROW0 - 30, sh_w))
    a('<text x="%d" y="%d" text-anchor="middle" class="shieldtxt">ESP32-S3</text>'
      % (sh_x + sh_w // 2, ROW0 + 10))
    for i, line in enumerate(["Waveshare", "ESP32-S3-ETH", "16 МБ flash",
                              "8 МБ PSRAM", "W5500"]):
        a('<text x="%d" y="%d" text-anchor="middle" class="shieldsml">%s</text>'
          % (sh_x + sh_w // 2, ROW0 + 30 + i * (26 if i < 2 else 24), line))

    # ---- пины
    for side, pins in (("L", LEFT), ("R", RIGHT)):
        for idx, (contact, name) in enumerate(pins):
            y = row_y(idx)
            is_psram = contact in PSRAM_CONTACTS
            conn = CONN.get(contact)
            if side == "L":
                num_x, num_anchor = BOARD_X + 16, "start"
                name_x, name_anchor = BOARD_X + 46, "start"
                band_x = BOARD_X + 6
                stub_a, stub_b = BOARD_X - 2, BOARD_X - 18
            else:
                num_x, num_anchor = BOARD_X1 - 16, "end"
                name_x, name_anchor = BOARD_X1 - 46, "end"
                band_x = BOARD_X1 - 158
                stub_a, stub_b = BOARD_X1 + 2, BOARD_X1 + 18
            if conn:
                a('<rect x="%d" y="%d" width="152" height="46" rx="6" fill="%s" '
                  'opacity="0.30"/>' % (band_x, y - 23, conn[0]))
            a('<line x1="%d" y1="%d" x2="%d" y2="%d" class="stub"/>'
              % (stub_a, y, stub_b, y))
            if conn:
                a('<circle cx="%d" cy="%d" r="8" fill="%s" stroke="#ffffff" '
                  'stroke-width="1.6"/>' % (stub_b, y, conn[0]))
                cls_no, fill_no = "pinno", conn[0]
            elif is_psram:
                cls_no, fill_no = "dimno", "#9aa7b4"
            else:
                cls_no, fill_no = "dimno", "#7d8894"
            a('<text x="%d" y="%d" text-anchor="%s" class="%s" fill="%s">%d</text>'
              % (num_x, y + 5, num_anchor, cls_no, fill_no, contact))
            if is_psram:
                cls_nm = "psramname"
            elif conn:
                cls_nm = "usedname"
            else:
                cls_nm = "dimname"
            a('<text x="%d" y="%d" text-anchor="%s" class="%s">%s</text>'
              % (name_x, y + 6, name_anchor, cls_nm, esc(name)))

    # ---- подписи снаружи платы
    pad, gap = 8, 18
    for side, pins in (("L", LEFT), ("R", RIGHT)):
        x0 = COL_LEFT_X if side == "L" else COL_RIGHT_X
        x1 = x0 + COL_BLOCK_W
        prev_bottom = None
        for idx, (contact, _name) in enumerate(pins):
            conn = CONN.get(contact)
            if not conn:
                continue
            color, head, func = conn
            lines = wrap(func, 15, COL_BLOCK_W - 2 * pad)
            h = pad + 13 + gap * len(lines) + 5 + pad
            y = row_y(idx)
            top = y - h / 2
            if prev_bottom is not None and top < prev_bottom + 6:
                top = prev_bottom + 6
            prev_bottom = top + h
            bx = x1 if side == "L" else x0
            px = BOARD_X - 18 if side == "L" else BOARD_X1 + 18
            mid = (bx + px) / 2
            by = top + h / 2
            a('<polyline class="lead" points="%.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f"/>'
              % (bx, by, mid, by, mid, y, px, y))
            a('<rect x="%.1f" y="%.1f" width="%d" height="%.1f" rx="7" fill="#ffffff" '
              'stroke="%s" stroke-width="2"/>' % (x0, top, COL_BLOCK_W, h, color))
            a('<rect x="%.1f" y="%.1f" width="6" height="%.1f" rx="3" fill="%s"/>'
              % (x0 if side == "L" else x1 - 6, top + 3, h - 6, color))
            ty = top + pad + 12
            a('<text x="%.1f" y="%.1f" class="blkt1" fill="%s">%s</text>'
              % (x0 + pad, ty, color, esc(head)))
            for k, line in enumerate(lines):
                a('<text x="%.1f" y="%.1f" class="blkt2">%s</text>'
                  % (x0 + pad, ty + gap * (k + 1), esc(line)))

    # ---- метка PSRAM
    ps_top = row_y(0) - PITCH / 2 + 4
    ps_bot = row_y(5) + PITCH / 2 - 4
    pxl = BOARD_X - 46
    a('<path d="M %.1f %.1f L %.1f %.1f L %.1f %.1f L %.1f %.1f" fill="none" '
      'stroke="#6f7a86" stroke-width="2"/>'
      % (pxl + 8, ps_top, pxl, ps_top, pxl, ps_bot, pxl + 8, ps_bot))
    cy = (ps_top + ps_bot) / 2
    a('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="#6f7a86" '
      'stroke-width="1.4"/>' % (pxl, cy, pxl - 10, cy))
    a('<text x="%.1f" y="%.1f" text-anchor="end" class="psramtxt" fill="#5f6a77">'
      'PSRAM</text>' % (pxl - 14, cy - 12))
    a('<text x="%.1f" y="%.1f" text-anchor="end" class="psramsml" fill="#5f6a77">'
      'IO33…IO37</text>' % (pxl - 14, cy + 8))
    a('<text x="%.1f" y="%.1f" text-anchor="end" class="psramsml" fill="#5f6a77">'
      'не использовать</text>' % (pxl - 14, cy + 27))

    # ---- легенда
    lg_y = BOARD_Y1 + 110
    total = sum(34 + text_w(t, 15) + 34 for t, _c in LEGEND)
    lx = (W - total) / 2
    for text, color in LEGEND:
        a('<rect x="%.1f" y="%d" width="26" height="6" rx="3" fill="%s"/>'
          % (lx, lg_y - 9, color))
        a('<text x="%.1f" y="%d" class="note">%s</text>' % (lx + 34, lg_y, esc(text)))
        lx += 34 + text_w(text, 15) + 34

    # ---- примечания
    ny = BOARD_Y1 + 162
    a('<line x1="18" y1="%d" x2="%d" y2="%d" class="sepline"/>' % (ny - 26, W - 18, ny - 26))
    a('<text x="18" y="%d" class="grouplbl" fill="#2b3543">ПРИМЕЧАНИЯ</text>' % ny)
    for i, n in enumerate(NOTES):
        y = ny + 26 + i * 27
        a('<circle cx="22" cy="%.1f" r="3.2" class="notebullet"/>' % (y - 5))
        a('<text x="34" y="%d" class="note">%s</text>' % (y, esc(n)))

    a('</svg>')
    return "\n".join(out) + "\n"


def main():
    try:
        validate()
    except AssertionError as exc:
        sys.stderr.write("ОШИБКА ДАННЫХ: %s\nSVG не записан.\n" % exc)
        return 1
    svg = build()
    with open(OUT, "w", encoding="utf-8") as f:
        f.write(svg)
    print(OUT)
    return 0


if __name__ == "__main__":
    sys.exit(main())
