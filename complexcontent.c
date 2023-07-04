#include "complexcontent.h"

cComplexContent::cComplexContent() {
  Osd = NULL;
  Pixmap = NULL;
  PixmapImage = NULL;
  isShown = false;
  isScrollingActive = true;
}

cComplexContent::cComplexContent(cOsd *osd, int scrollSize) {
  Osd = osd;
  ScrollSize = scrollSize;

  Pixmap = NULL;
  PixmapImage = NULL;
  isShown = false;
  isScrollingActive = true;
}

cComplexContent::~cComplexContent() {}

void cComplexContent::Clear(void) {
  isShown = false;
  Contents.clear();
  if (Osd) {
    if (Pixmap != NULL) {
      Osd->DestroyPixmap(Pixmap);
      Pixmap = NULL;
    }
    if (PixmapImage != NULL) {
      Osd->DestroyPixmap(PixmapImage);
      PixmapImage = NULL;
    }
  }
}

cPixmap *cComplexContent::CreatePixmap(int Layer, const cRect &ViewPort, const cRect &DrawPort) {
  cSize maxPixmapSize = Osd->MaxPixmapSize();
  cRect SafeDrawPort(DrawPort.X(), DrawPort.Y(), DrawPort.Width(), DrawPort.Height());

  if (DrawPort.Width() > maxPixmapSize.Width()) {
    dsyslog("Try to create Pixmap (%d x %d) > MaxPixmapSize (%d x %d)-> cut Pixmap to MaxPixmapSize",
            DrawPort.Width(), DrawPort.Height(), maxPixmapSize.Width(), maxPixmapSize.Height());
    SafeDrawPort.SetWidth(maxPixmapSize.Width());
  }
  if (DrawPort.Height() > maxPixmapSize.Height()) {
    dsyslog("Try to create Pixmap (%d x %d) > MaxPixmapSize (%d x %d)-> cut Pixmap to MaxPixmapSize",
            DrawPort.Width(), DrawPort.Height(), maxPixmapSize.Width(), maxPixmapSize.Height());
    SafeDrawPort.SetHeight(maxPixmapSize.Height());
  }

  return Osd->CreatePixmap(Layer, ViewPort, SafeDrawPort);
}

void cComplexContent::CreatePixmaps(bool fullFillBackground) {
  CalculateDrawPortHeight();
  FullFillBackground = fullFillBackground;

  if (!Osd)
    return;
  if (Pixmap != NULL) {
    Osd->DestroyPixmap(Pixmap);
    Pixmap = NULL;
  }
  if (PixmapImage != NULL) {
    Osd->DestroyPixmap(PixmapImage);
    PixmapImage = NULL;
  }

  cRect PositionDraw;
  PositionDraw.SetLeft(0);
  PositionDraw.SetTop(0);
  PositionDraw.SetWidth(Position.Width());
  if (FullFillBackground && DrawPortHeight < Position.Height())
    PositionDraw.SetHeight(Position.Height());
  else
    PositionDraw.SetHeight(DrawPortHeight);

  Pixmap = CreatePixmap(1, Position, PositionDraw);
  PixmapImage = CreatePixmap(2, Position, PositionDraw);
  // dsyslog("skinflatplus: ComplexContentPixmap left: %d top: %d width: %d height: %d", Position.Left(), Position.Top(), Position.Width(),
  // Position.Height() ); dsyslog("skinflatplus: ComplexContentPixmap drawport left: %d top: %d width: %d height: %d", PositionDraw.Left(),
  // PositionDraw.Top(), PositionDraw.Width(), PositionDraw.Height() );

  if (FullFillBackground) {
    Pixmap->Fill(ColorBg);
  } else {
    Pixmap->DrawRectangle(cRect(0, 0, Position.Width(), ContentHeight(false)),
                          ColorBg);
  }

  PixmapImage->Fill(clrTransparent);
}

void cComplexContent::CalculateDrawPortHeight(void) {
  DrawPortHeight = 0;
  std::vector<cSimpleContent>::iterator it;
  for (it = Contents.begin(); it != Contents.end(); it++) {
    if ((*it).GetBottom() > DrawPortHeight)
      DrawPortHeight = (*it).GetBottom();
  }
  if (isScrollingActive)
    DrawPortHeight = ScrollTotal() * ScrollSize;
}

int cComplexContent::BottomContent(void) {
  int bottom = 0;
  std::vector<cSimpleContent>::iterator it;
  for (it = Contents.begin(); it != Contents.end(); it++) {
    if ((*it).GetBottom() > bottom)
      bottom = (*it).GetBottom();
  }
  return bottom;
}

int cComplexContent::ContentHeight(bool Full) {
  if (Full)
    return Height();
  CalculateDrawPortHeight();
  if (DrawPortHeight > Height())
    return Height();

  return DrawPortHeight;
}

bool cComplexContent::Scrollable(int height) {
  CalculateDrawPortHeight();

  int total = 0, shown = 0;
  total = ScrollTotal();
  if (height == 0)
    height = Position.Height();
  shown = ceil((double)height / (double)ScrollSize);
  if (total > shown)
    return true;

  return false;
}

void cComplexContent::AddText(const char *text, bool multiline, cRect position, tColor colorFg, tColor colorBg, cFont *font,
                              int textWidth, int textHeight, int textAlignment) {
  Contents.push_back(cSimpleContent());
  Contents.back().SetText(text, multiline, position, colorFg, colorBg, font, textWidth, textHeight, textAlignment);
}

void cComplexContent::AddImage(cImage *image, cRect position) {
  Contents.push_back(cSimpleContent());
  Contents.back().SetImage(image, position);
}

void cComplexContent::AddImageWithFloatedText(cImage *image, int imageAlignment, const char *text, cRect textPos,
                                              tColor colorFg, tColor colorBg, cFont *font, int textWidth,
                                              int textHeight, int textAlignment) {
  int TextWidthLeft = Position.Width() - image->Width() - 10 - textPos.Left();

  cTextWrapper WrapperFloat;
  WrapperFloat.Set(text, font, TextWidthLeft);
  int FloatLines = ceil((double)image->Height() / (double)ScrollSize);
  int Lines = WrapperFloat.Lines();

  if (Lines < FloatLines) {
    cRect FloatedTextPos;
    FloatedTextPos.SetLeft(textPos.Left());
    FloatedTextPos.SetTop(textPos.Top());
    FloatedTextPos.SetWidth(TextWidthLeft);
    FloatedTextPos.SetHeight(textPos.Height());

    AddText(text, true, FloatedTextPos, colorFg, colorBg, font, textWidth, textHeight, textAlignment);

    cRect ImagePos;
    ImagePos.SetLeft(textPos.Left() + TextWidthLeft + 5);
    ImagePos.SetTop(textPos.Top());
    ImagePos.SetWidth(image->Width());
    ImagePos.SetHeight(image->Height());

    AddImage(image, ImagePos);
  } else {
    int NumChars = 0;
    for (int i = 0; i < Lines && i < FloatLines; i++) {
      NumChars += strlen(WrapperFloat.GetLine(i));
    }
    // detect end of last word
    for (; text[NumChars] != ' ' && text[NumChars] != '\0' &&
           text[NumChars] != '\r' && text[NumChars] != '\n';
         NumChars++)
      ;
    char *FloatedText;
    FloatedText = new char[NumChars + 1];
    memset(FloatedText, '\0', NumChars + 1);
    strncpy(FloatedText, text, NumChars);

    NumChars++;
    char *SecondText;
    SecondText = new char[strlen(text) - NumChars + 2];
    memset(SecondText, '\0', strlen(text) - NumChars + 2);
    strncpy(SecondText, text + NumChars, strlen(text) - NumChars);

    cRect FloatedTextPos;
    FloatedTextPos.SetLeft(textPos.Left());
    FloatedTextPos.SetTop(textPos.Top());
    FloatedTextPos.SetWidth(TextWidthLeft);
    FloatedTextPos.SetHeight(textPos.Height());

    cRect SecondTextPos;
    SecondTextPos.SetLeft(textPos.Left());
    SecondTextPos.SetTop(textPos.Top() + FloatLines * ScrollSize);
    SecondTextPos.SetWidth(textPos.Width());
    SecondTextPos.SetHeight(textPos.Height());

    AddText(FloatedText, true, FloatedTextPos, colorFg, colorBg, font, textWidth, textHeight, textAlignment);
    AddText(SecondText, true, SecondTextPos, colorFg, colorBg, font, textWidth, textHeight, textAlignment);

    cRect ImagePos;
    ImagePos.SetLeft(textPos.Left() + TextWidthLeft + 5);
    ImagePos.SetTop(textPos.Top());
    ImagePos.SetWidth(image->Width());
    ImagePos.SetHeight(image->Height());

    AddImage(image, ImagePos);

    delete[] FloatedText;
    delete[] SecondText;
  }
}

void cComplexContent::AddRect(cRect position, tColor colorBg) {
  Contents.push_back(cSimpleContent());
  Contents.back().SetRect(position, colorBg);
}

void cComplexContent::Draw() {
  isShown = true;
  std::vector<cSimpleContent>::iterator it;
  for (it = Contents.begin(); it != Contents.end(); it++) {
    if ((*it).GetContentType() == CT_Image)
      (*it).Draw(PixmapImage);
    else
      (*it).Draw(Pixmap);
  }
}

double cComplexContent::ScrollbarSize(void) {
  double barSize = (double)Position.Height() / (double)DrawPortHeight;
  return barSize;
}

int cComplexContent::ScrollTotal(void) {
  int total = ceil((double)DrawPortHeight / (double)ScrollSize);
  return total;
}

int cComplexContent::ScrollShown(void) {
  // int shown = ceil( (double)Position.Height() / (double)ScrollSize);
  int shown = Position.Height() / ScrollSize;
  return shown;
}

int cComplexContent::ScrollOffset(void) {
  double offset;

  int y = Pixmap->DrawPort().Point().Y() * -1;
  if (y + Position.Height() + ScrollSize > DrawPortHeight) {
    if (y == DrawPortHeight - Position.Height()) {
      y += ScrollSize;
    } else {
      y = DrawPortHeight - Position.Height() - 1;
    }
  }
  offset = (double)y / (double)DrawPortHeight;
  return ScrollTotal() * offset;
}

bool cComplexContent::Scroll(bool Up, bool Page) {
  int aktHeight = Pixmap->DrawPort().Point().Y();
  int totalHeight = Pixmap->DrawPort().Height();
  int screenHeight = Pixmap->ViewPort().Height();
  int lineHeight = ScrollSize;

  bool scrolled = false;
  if (Up) {
    if (Page) {
      int newY = aktHeight + screenHeight;
      if (newY > 0)
        newY = 0;
      Pixmap->SetDrawPortPoint(cPoint(0, newY));
      PixmapImage->SetDrawPortPoint(cPoint(0, newY));
      scrolled = true;
    } else {
      if (aktHeight < 0) {
        if (aktHeight + lineHeight < 0) {
          Pixmap->SetDrawPortPoint(cPoint(0, aktHeight + lineHeight));
          PixmapImage->SetDrawPortPoint(cPoint(0, aktHeight + lineHeight));
        } else {
          Pixmap->SetDrawPortPoint(cPoint(0, 0));
          PixmapImage->SetDrawPortPoint(cPoint(0, 0));
        }
        scrolled = true;
      }
    }
  } else {
    if (Page) {
      int newY = aktHeight - screenHeight;
      if ((-1) * newY > totalHeight - screenHeight)
        newY = (-1) * (totalHeight - screenHeight);
      Pixmap->SetDrawPortPoint(cPoint(0, newY));
      PixmapImage->SetDrawPortPoint(cPoint(0, newY));
      scrolled = true;
    } else {
      if (totalHeight - ((-1) * aktHeight + lineHeight) > screenHeight) {
        Pixmap->SetDrawPortPoint(cPoint(0, aktHeight - lineHeight));
        PixmapImage->SetDrawPortPoint(cPoint(0, aktHeight - lineHeight));
      } else {
        int newY = aktHeight - screenHeight;
        if ((-1) * newY > totalHeight - screenHeight)
          newY = (-1) * (totalHeight - screenHeight);
        Pixmap->SetDrawPortPoint(cPoint(0, newY));
        PixmapImage->SetDrawPortPoint(cPoint(0, newY));
      }
      scrolled = true;
    }
  }

  return scrolled;
}
