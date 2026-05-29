import re, os, openpyxl
from openpyxl.styles import Font, PatternFill, Alignment, Border, Side
from openpyxl.utils import get_column_letter

# --- PARSER ---
with open('results_v0.2.txt', 'r') as f:
    content = f.read()

parts = re.split(r'={30,}\nSCENARIO:\s*(\w+)\n={30,}', content)
scenarios_raw = {parts[i].strip(): parts[i+1] for i in range(1, len(parts), 2)}

def extract_field(label, text):
    m = re.search(rf'\n{re.escape(label)}\n((?:.*\n?)*?)(?=\n[A-Z\(]|\Z)', text)
    return [l.strip() for l in m.group(1).split('\n') if l.strip()] if m else []

def parse_scenario(block):
    fields = {
        'names': extract_field('Instance name', block),
        'seeds': extract_field('Seed', block),
        'makespans': extract_field('Makespan', block),
        'cpu_s': extract_field('CPU Time (s)', block),
        'cpu_min': extract_field('CPU Time (min)', block),
        'release': extract_field('Release time', block),
        'inbound': extract_field('Inbound sequence', block),
        'outbound': extract_field('Outbound sequence', block),
    }
    rows = []
    g = lambda lst, i: lst[i] if i < len(lst) else ''
    for i in range(len(fields['names'])):
        name = g(fields['names'], i)
        mm = re.search(r'_n(\d+)_m(\d+)', name)
        n_val = int(mm.group(1)) if mm else ''
        m_val = int(mm.group(2)) if mm else ''
        cs = g(fields['cpu_s'], i); cm = g(fields['cpu_min'], i); ms = g(fields['makespans'], i)
        try: ms = int(ms)
        except: pass
        try: cs = float(cs)
        except: pass
        try: cm = float(cm)
        except: cm = ''
        rows.append([name, g(fields['seeds'],i), n_val, m_val, ms, cs, cm,
                     g(fields['release'],i), g(fields['inbound'],i), g(fields['outbound'],i)])
    return rows

# --- EXCEL WRITER ---
HEADERS = ['Istanza','Seed','n (inbound)','m (outbound)','Makespan',
           'CPU Time (s)','CPU Time (min)','release time','Inbound sequence','Outbound sequence']
HEADER_FILL = PatternFill("solid", fgColor="4F81BD")
HEADER_FONT = Font(bold=True, color="FFFFFF", name="Calibri", size=11)
EVEN_FILL   = PatternFill("solid", fgColor="DCE6F1")
ODD_FILL    = PatternFill("solid", fgColor="FFFFFF")
thin = Side(style="thin", color="B8CCE4")
border = Border(left=thin, right=thin, top=thin, bottom=thin)
COL_WIDTHS = [40, 8, 12, 14, 12, 14, 14, 55, 55, 55]

wb = openpyxl.Workbook()
wb.remove(wb.active)
for sc_name, block in scenarios_raw.items():
    ws = wb.create_sheet(title=sc_name)
    for ci, h in enumerate(HEADERS, 1):
        c = ws.cell(row=1, column=ci, value=h)
        c.font = HEADER_FONT; c.fill = HEADER_FILL
        c.alignment = Alignment(horizontal="center", vertical="center"); c.border = border
    for ri, row in enumerate(parse_scenario(block), 2):
        fill = EVEN_FILL if ri % 2 == 0 else ODD_FILL
        for ci, val in enumerate(row, 1):
            c = ws.cell(row=ri, column=ci, value=val)
            c.fill = fill; c.border = border
            c.alignment = Alignment(horizontal="left" if ci in (1,8,9,10) else "center", vertical="center")
    for ci, w in enumerate(COL_WIDTHS, 1):
        ws.column_dimensions[get_column_letter(ci)].width = w
    ws.freeze_panes = "A2"

os.makedirs('output', exist_ok=True)
wb.save('output/Cross-Docking-Results-AUTO.xlsx')
print("✅ Excel generato!")