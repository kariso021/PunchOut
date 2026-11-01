from pathlib import Path
from docx import Document
from docx.shared import Cm, Pt, RGBColor
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.enum.text import WD_ALIGN_PARAGRAPH, WD_LINE_SPACING
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_CELL_VERTICAL_ALIGNMENT
from PIL import Image

ROOT = Path(__file__).parent
doc = Document()
s = doc.sections[0]
s.page_width, s.page_height = Cm(21), Cm(29.7)
s.top_margin, s.bottom_margin = Cm(1.35), Cm(1.35)
s.left_margin, s.right_margin = Cm(1.8), Cm(1.8)
for name in ['Normal', 'Title', 'Heading 1', 'Heading 2']:
    st = doc.styles[name]
    st.font.name = '에스코어 드림 4 Regular'
    st._element.rPr.rFonts.set(qn('w:eastAsia'), '에스코어 드림 4 Regular')
    st.font.color.rgb = RGBColor(0, 0, 0)
    st.paragraph_format.space_after = Pt(5)
doc.styles['Normal'].font.size = Pt(9.5)
doc.styles['Normal'].paragraph_format.line_spacing = Pt(13)
doc.styles['Normal'].paragraph_format.line_spacing_rule = WD_LINE_SPACING.AT_LEAST
doc.styles['Normal'].paragraph_format.space_before = Pt(0)
doc.styles['Title'].font.size = Pt(25)
doc.styles['Heading 1'].font.size = Pt(14)
doc.styles['Heading 1'].paragraph_format.space_before = Pt(9)
doc.styles['Heading 2'].font.size = Pt(10.5)
doc.styles['Heading 2'].paragraph_format.space_before = Pt(6)
for name in ['Title', 'Heading 1', 'Heading 2']:
    st = doc.styles[name]
    st.font.name = '에스코어 드림 6 Bold'
    st._element.rPr.rFonts.set(qn('w:eastAsia'), '에스코어 드림 6 Bold')
    st.font.bold = False
    st.font.color.rgb = RGBColor.from_string('1646A0')
    st.paragraph_format.line_spacing = 1.08
for st in doc.styles:
    if st.type == 1:
        pp = st.element.get_or_add_pPr()
        for border in list(pp.findall(qn('w:pBdr'))): pp.remove(border)
        snap = OxmlElement('w:snapToGrid'); snap.set(qn('w:val'), '0'); pp.append(snap)

def set_font(run, name):
    run.font.name = name
    run._r.get_or_add_rPr().rFonts.set(qn('w:eastAsia'), name)

def bullet(para):
    np=OxmlElement('w:numPr')
    for tag,val in [('ilvl','0'),('numId','42')]:
        el=OxmlElement('w:'+tag); el.set(qn('w:val'),val); np.append(el)
    para._p.get_or_add_pPr().append(np)

numbering = doc.part.numbering_part.element
abstract = OxmlElement('w:abstractNum'); abstract.set(qn('w:abstractNumId'), '42')
lvl = OxmlElement('w:lvl'); lvl.set(qn('w:ilvl'), '0')
for tag, val in [('start','1'),('numFmt','bullet'),('lvlText','▪'),('lvlJc','left')]:
    el=OxmlElement('w:'+tag); el.set(qn('w:val'),val); lvl.append(el)
pp=OxmlElement('w:pPr'); indent=OxmlElement('w:ind')
indent.set(qn('w:left'),'190'); indent.set(qn('w:hanging'),'190'); pp.append(indent); lvl.append(pp)
rp=OxmlElement('w:rPr'); fonts=OxmlElement('w:rFonts')
fonts.set(qn('w:ascii'),'Segoe UI Symbol'); fonts.set(qn('w:hAnsi'),'Segoe UI Symbol'); rp.append(fonts)
color=OxmlElement('w:color'); color.set(qn('w:val'),'1646A0'); rp.append(color); lvl.append(rp)
abstract.append(lvl); numbering.append(abstract)
num=OxmlElement('w:num'); num.set(qn('w:numId'),'42')
aid=OxmlElement('w:abstractNumId'); aid.set(qn('w:val'),'42'); num.append(aid); numbering.append(num)

def p(text='', bold=False, size=None):
    if text.startswith('• '):
        for line in text.split('\n'):
            para=doc.add_paragraph()
            para.paragraph_format.space_after=Pt(3)
            bullet(para)
            para.add_run(line.removeprefix('• '))
        return para
    x = doc.add_paragraph()
    r = x.add_run(text)
    r.bold = bold
    if bold: set_font(r, '에스코어 드림 5 Medium'); r.bold = False
    if size: r.font.size = Pt(size)
    return x

def h(text):
    return doc.add_paragraph(text, 'Heading 1')

def table(headers, rows, widths):
    t = doc.add_table(rows=1 if headers else 0, cols=len(widths))
    t.alignment = WD_TABLE_ALIGNMENT.CENTER
    t.autofit = False
    for col, width in zip(t.columns, widths): col.width = Cm(width)
    if headers:
        for c, text in zip(t.rows[0].cells, headers): c.text = text
        t.rows[0]._tr.get_or_add_trPr().append(OxmlElement('w:tblHeader'))
    for row in rows:
        for c, text in zip(t.add_row().cells, row): c.text = text
    for i, row in enumerate(t.rows):
        pr = row._tr.get_or_add_trPr()
        pr.append(OxmlElement('w:cantSplit'))
        for j, c in enumerate(row.cells):
            c.width = Cm(widths[j])
            c.vertical_alignment = WD_CELL_VERTICAL_ALIGNMENT.CENTER
            cp = c._tc.get_or_add_tcPr()
            borders = OxmlElement('w:tcBorders')
            for edge in ['top', 'left', 'bottom', 'right']:
                el = OxmlElement('w:' + edge)
                for k,v in [('val','single'),('sz','4'),('color','D9D9D9')]: el.set(qn('w:'+k), v)
                borders.append(el)
            cp.append(borders)
            margins = OxmlElement('w:tcMar')
            for edge in ['top','bottom','left','right']:
                el = OxmlElement('w:'+edge); el.set(qn('w:w'), '55'); el.set(qn('w:type'),'dxa'); margins.append(el)
            cp.append(margins)
            if headers and i == 0:
                shade = OxmlElement('w:shd'); shade.set(qn('w:fill'),'EDF2FA'); cp.append(shade)
            for para in c.paragraphs:
                para.paragraph_format.space_after = Pt(0)
                para.paragraph_format.line_spacing = Pt(13)
                para.paragraph_format.line_spacing_rule = WD_LINE_SPACING.AT_LEAST
                if headers and i == 0:
                    para.alignment = WD_ALIGN_PARAGRAPH.CENTER
                    para.paragraph_format.keep_with_next = True
                    for run in para.runs:
                        set_font(run, '에스코어 드림 5 Medium')
                        run.bold = False
                        run.font.color.rgb = RGBColor.from_string('1646A0')
    if headers and headers[0] == '기술':
        labels = [
            ['메모리 관리', '코드 재사용', '모듈 설계'],
            ['렌더러 제작', '그래픽스 기초'],
            ['엔진 활용', 'AI 구현', '공격권 관리', 'UI 및 도구 제작'],
            ['UI 아키텍처', '멀티플레이', '성능 최적화', '비동기 처리'],
            ['게임 제작', 'AI 및 이미지 변환'],
        ]
        for row, names in zip(t.rows[1:], labels):
            cell=row.cells[2]
            lines=cell.text.split('\n')
            cell.text=''
            for i,(label,line) in enumerate(zip(names,lines)):
                para=cell.paragraphs[0] if i==0 else cell.add_paragraph()
                bullet(para)
                para.paragraph_format.space_after=Pt(1)
                para.paragraph_format.line_spacing=Pt(13)
                para.paragraph_format.line_spacing_rule=WD_LINE_SPACING.AT_LEAST
                lead=para.add_run(label+'  ')
                set_font(lead, '에스코어 드림 5 Medium')
                lead.font.color.rgb = RGBColor.from_string('1646A0')
                para.add_run(line)
    return t

doc.add_paragraph('황인규 이력서', 'Title')
p('게임 클라이언트 · UI 시스템 개발', True, 11)
info = table(None, [
    ['이름', '황인규'], ['연락처', '010-8445-3827'],
    ['이메일', 'dlsrb021@gmail.com'], ['GitHub', 'https://github.com/kariso021'],
    ['주소', '서울시 관악구 봉천동 Premio-C 301'],
], [2.1, 15.3])
# Preserve the original portrait, cropping only its surrounding white page area.
im = Image.open(ROOT/'source_image.jpeg')
im.crop((294, 483, 674, 957)).save(ROOT/'portrait.png')
for row in info.rows:
    row.cells[1].width = Cm(11.9)
photo_col = info.add_column(Cm(3.4))
photo = info.cell(0,2).merge(info.cell(4,2))
photo.vertical_alignment = WD_CELL_VERTICAL_ALIGNMENT.CENTER
photo.paragraphs[0].alignment = WD_ALIGN_PARAGRAPH.CENTER
photo.paragraphs[0].paragraph_format.line_spacing = 1.0
photo.paragraphs[0].add_run().add_picture(str(ROOT/'portrait.png'), width=Cm(2.6))
for row in info.rows: row.cells[0].width=Cm(2.1); row.cells[1].width=Cm(11.9)
info.columns[0].width=Cm(2.1); info.columns[1].width=Cm(11.9); info.columns[2].width=Cm(3.4)
h('소개')
p('문제의 핵심을 파악하고 명확한 우선순위에 따라 효율적으로 해결하는 개발자입니다. 약속과 일정을 지키는 책임감, 즐겁게 소통하는 태도를 바탕으로 동료와 신뢰를 쌓고 함께 성장합니다.')
h('경력 사항')
table(['근무 기간', '회사', '계약 형태 및 업무'], [['2025.11 ~ 현재', 'EA Korea', 'Adecco 파견 계약\n차기작 개발 참여']], [3.6, 5.4, 8.4])
doc.add_paragraph('UI 시스템 및 주요 기능 개발', 'Heading 2')
p('• Frostbite 엔진 기반의 핵심 UI 시스템과 신규 UI 기능 개발\n• TA(Technical Artist)와 협업해 기능 및 시각적 연출의 기술 요구사항 반영\n• 3D 게임 요소와 2D UI 간 상호작용 및 연동 기능 구현')
doc.add_paragraph('아키텍처 개선 및 안정성 확보', 'Heading 2')
p('• 레거시 시스템을 분석하고 도메인 특성에 맞게 바인딩 구조 명확화 및 이벤트 최소화\n• MVVM 패턴으로 View와 ViewModel 간 결합도를 낮추고 부수 효과 방지')
p().paragraph_format.space_after = Pt(1)
table(['수행 기간', '프로젝트', '개발 형태'], [['3개월', 'Musium Project', '1인 풀스택 외주 개발']], [3.6, 5.4, 8.4])
doc.add_paragraph('주요 담당 업무', 'Heading 2')
p('• 3D 상호작용: LineTrace·BoxCollision으로 월드 내 유물 오브젝트를 탐지하고 Hover·Select 인터랙션 구현\n• 커스텀 UI: C++ Slate와 스마트 포인터 기반의 라디오 버튼 위젯 개발\n• 비동기 로딩: Data Table의 대용량 텍스트·에셋 경로를 TSoftObjectPtr로 비동기 로딩해 UI 생성 시 끊김 방지\n• 데이터 처리: C++ 서브시스템의 로그 수집·저장과 에디터 유틸리티 위젯의 CSV 추출 기능 연계')

doc.add_page_break()
h('기술 스택')
table(['기술', '수준', '활용 경험 및 역량'], [
 ['C / C++', '중급', '스마트 포인터를 활용한 순환 참조 해소 및 객체 생명주기 관리\n자료구조·알고리즘의 선택적 활용과 상속·템플릿 기반 코드 재사용\n인터페이스·델리게이트 기반 모듈화 및 확장 가능한 코드 설계'],
 ['DirectX', '초급', 'Rastertek 튜토리얼 기반 체스 게임 렌더러 제작\n그래픽스 파이프라인 학습 및 구현'],
 ['Unreal Engine', '중급', '기본 클래스 구조와 UPROPERTY 이해 및 엔진 헤더 기반 함수 활용\nBehavior Tree 기반 AI 4종 구현(MELEE·RANGED·MAGE·BOSS)\n세마포어 원리를 응용한 AI 공격권(Token) 관리 시스템 설계·구현\nUMG·C++ Slate 기반 상호작용 커스텀 툴 제작'],
 ['Unity 및\n아키텍처', '중급', 'MVVM 기반 UI·시스템 아키텍처 설계 및 뷰·데이터 결합도 분리\nReliable UDP 기반 멀티플레이 서버·클라이언트 아키텍처 구현\n모바일 프로파일링 및 비동기 처리 최적화\nC# async/await, .NET Task, Coroutine, NGO 비동기 이벤트 활용'],
 ['Python', '초급', 'Command Pattern 기반 1945 스타일 슈팅 게임 및 리플레이 구현\nPyTorch·CycleGAN 기반 모델 학습 및 이미지 스타일 변환 프로젝트'],
], [3,1.3,13.1])
h('교육 및 병역')
table(['기간', '구분', '내용'], [
 ['2019.02 ~ 2025.02', '학력', '홍익대학교 세종캠퍼스 게임학부\n게임소프트웨어 졸업'],
 ['2020.08 ~ 2022.02', '병역', '대한민국 육군 만기 전역'],
], [4.3,1.8,11.3])
h('수상 내역')
table(['연도', '수상 내용', '프로젝트'], [
 ['2020', '홍익대학교 게임학부 프로토타입 우수상', '별의별 분식'],
 ['2025', '홍익대학교 게임학부 졸업전시 인기상\n최다득표상', 'LumenStrike'],
], [1.8,11.3,4.3])
h('활동 내역')
table(['기간', '활동'], [
 ['2019.02 ~ 2021.02', '홍익대학교 게임학부 게임개발동아리 EXDIO 활동'],
 ['2022.02 ~ 2023.02', '홍익대학교 농구소모임 인피니티 회장'],
], [4.3,13.1])

doc.add_page_break()
h('자기소개서')
doc.add_paragraph('끝까지 파고들고 복기하는 태도', 'Heading 2')
p('어릴 때부터 퍼즐 하나를 풀기 위해 오랫동안 고민하며, 끝까지 파고드는 과정에서 성취감을 느꼈습니다. 중학교 시절에는 온라인 오목 대국을 약 1만 판 진행하며 전국 랭킹 100위권에 진입했고, 101연승을 달성했습니다. 패배할 때마다 원인을 분석하고 대국을 복기하는 습관이 실력을 키우는 바탕이 되었습니다.')
p('고등학교 때도 같은 태도로 리그 오브 레전드와 배틀그라운드의 플레이를 복기하며 다이아몬드와 그랜드마스터 등급을 달성했습니다. 게임을 좋아해 게임학부에 진학했지만, 대학 1학년 때는 플레이에 많은 시간을 쏟으며 학업과 자기 개발의 균형을 놓치기도 했습니다.')
doc.add_paragraph('꾸준한 실천으로 바꾼 개발 습관', 'Heading 2')
p('전환점은 군 복무 중 만난 선임이었습니다. 매일 새벽에 일어나 운동하고, 정해진 시간에 공부하며, 주변 동료까지 챙기는 모습을 보며 저도 제 분야에서 꾸준히 성장하고 싶다는 목표를 세웠습니다. C++ 책을 구입해 사이버지식정보방의 웹 IDE로 첫 코드를 작성했고, 전역 후에도 매일 코딩을 이어갔습니다.')
p('졸업작품 제출 1년 전에는 졸업 심사 최다 득표를 목표로 준비를 시작했습니다. 수업이 시작되기 4개월 전부터 Unreal Engine 강의를 듣고 선배들의 프로젝트를 분석하며 구조를 설계했습니다. C++ 데모를 만들어 구현 난이도를 점검했고, 발표가 가까워질수록 완성도를 높이는 데 집중했습니다. 그 결과 졸업작품 LumenStrike는 학우들의 최다 투표를 받아 1위에 선정되었고, 졸업전시 인기상을 받았습니다.')
doc.add_paragraph('피드백을 결과물의 개선으로 연결', 'Heading 2')
p('LumenStrike를 준비하며 코드를 공개하고 피드백을 받을수록 성장할 수 있다는 사실을 배웠습니다. 2주마다 진행되는 발표에서 부족한 코드를 보여주는 일은 부담스러웠지만, 피하지 않고 피드백을 바탕으로 문제를 하나씩 보완했습니다. 결과물이 개선될 때마다 성취감을 느꼈고, 이후에도 제 코드를 점검하고 다듬는 태도를 이어가고 있습니다.')
p('저는 끈기와 집요함을 바탕으로 문제를 분석하고, 계획을 실천하며, 피드백을 개선으로 연결해 왔습니다. 앞으로도 새로운 문제에 도전하며 합리적으로 해결책을 찾는 개발자로 성장하겠습니다.')

doc.core_properties.title = '황인규 이력서'
doc.core_properties.subject = '게임 클라이언트 및 UI 시스템 개발 이력서'
doc.core_properties.author = '황인규'
in_intro = False
for para in doc.paragraphs:
    if para.text == '자기소개서': in_intro = True
    if in_intro and para.style.name == 'Normal' and para.text:
        para.paragraph_format.line_spacing = Pt(16)
        para.paragraph_format.line_spacing_rule = WD_LINE_SPACING.AT_LEAST
        para.paragraph_format.space_after = Pt(8)
        for run in para.runs: run.font.size = Pt(10.5)
    pp = para._p.get_or_add_pPr()
    for border in list(pp.findall(qn('w:pBdr'))): pp.remove(border)
doc.save(ROOT/'이력서_황인규_정리본.docx')
print(ROOT/'이력서_황인규_정리본.docx')
