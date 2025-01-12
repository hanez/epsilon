#ifndef SOLVER_SOLUTIONS_CONTROLLER_H
#define SOLVER_SOLUTIONS_CONTROLLER_H

#include <apps/i18n.h>
#include <apps/shared/scrollable_two_layouts_cell.h>
#include <escher/alternate_empty_view_controller.h>
#include <escher/even_odd_buffer_text_cell.h>
#include <escher/even_odd_expression_cell.h>
#include <escher/selectable_table_view.h>
#include <escher/selectable_table_view_data_source.h>
#include <escher/selectable_table_view_delegate.h>
#include <escher/tab_view_controller.h>
#include <escher/table_view_data_source.h>
#include <ion.h>

#include "equation_store.h"
#include "system_of_equations.h"

namespace Solver {

class SolutionsController : public Escher::ViewController,
                            public Escher::AlternateEmptyViewDelegate,
                            public Escher::SelectableTableViewDataSource,
                            public Escher::TableViewDataSource,
                            public Escher::SelectableTableViewDelegate {
 public:
  SolutionsController(Escher::Responder *parentResponder);

  // ViewController
  const char *title() override;
  Escher::View *view() override { return &m_contentView; }
  void viewWillAppear() override;
  void viewDidDisappear() override;
  void didEnterResponderChain(
      Escher::Responder *previousFirstResponder) override;
  TELEMETRY_ID("Solutions");

  // AlternateEmptyViewDelegate
  bool isEmpty() const override { return false; }  // View cannot be empty
  I18n::Message emptyMessage() override {
    assert(false);
    return static_cast<I18n::Message>(0);
  }
  Escher::Responder *responderWhenEmpty() override;

  // TableViewDataSource
  int numberOfRows() const override;
  int numberOfColumns() const override { return 2; }
  void fillCellForLocation(Escher::HighlightCell *cell, int column,
                           int row) override;
  Escher::HighlightCell *reusableCell(int index, int type) override;
  int reusableCellCount(int type) override;
  int typeAtLocation(int column, int row) override;
  bool cellAtLocationIsSelectable(Escher::HighlightCell *cell, int column,
                                  int row) override {
    return typeAtLocation(column, row) != k_messageCellType &&
           typeAtLocation(column, row) != k_emptyCellType;
  }

  // Responder
  void didBecomeFirstResponder() override;

  // SelectableTableViewDelegate
  bool canStoreContentOfCellAtLocation(Escher::SelectableTableView *t, int col,
                                       int row) const override {
    return col > 0;
  }

 private:
  // TableViewDataSource
  KDCoordinate nonMemoizedColumnWidth(int column) override {
    return column == 0 ? k_symbolCellWidth : k_valueCellWidth;
  }
  // TODO: Memoize the row height ?
  KDCoordinate nonMemoizedRowHeight(int row) override;
  Escher::TabViewController *tabController() const {
    return static_cast<Escher::TabViewController *>(
        parentResponder()->parentResponder());
  }

  class ContentView : public Escher::View {
   public:
    constexpr static KDCoordinate k_topMargin = 50;
    constexpr static KDColor k_backgroundColor =
        Escher::Palette::WallScreenDark;
    ContentView(SolutionsController *controller);
    void drawRect(KDContext *ctx, KDRect rect) const override;
    void setWarning(bool warning);
    void setWarningMessages(I18n::Message message0, I18n::Message message1);
    Escher::SelectableTableView *selectableTableView() {
      return &m_selectableTableView;
    }

   private:
    constexpr static KDFont::Size k_warningMessageFont = KDFont::Size::Small;
    constexpr static KDGlyph::Format k_warningFormat{
        .style = {.backgroundColor = k_backgroundColor,
                  .font = k_warningMessageFont},
        .horizontalAlignment = KDGlyph::k_alignCenter};

    bool hideTableView() const {
      return m_selectableTableView.numberOfDisplayableRows() == 0;
    }
    int numberOfSubviews() const override;
    Escher::View *subviewAtIndex(int index) override;
    void layoutSubviews(bool force = false) override;
    Escher::MessageTextView m_warningMessageView0;
    Escher::MessageTextView m_warningMessageView1;
    Escher::SelectableTableView m_selectableTableView;
    bool m_displayWarningMoreSolutions;
  };

  class MessageCell : public Escher::HighlightCell {
   public:
    MessageCell()
        : m_messageView(
              (I18n::Message)0,
              {.style =
                   {.backgroundColor =
                        SolutionsController::ContentView::k_backgroundColor,
                    .font = KDFont::Size::Small},
               .verticalAlignment = k_verticalAlignment}) {}

    void setBackgroundColor(KDColor color) {
      m_messageView.setBackgroundColor(color);
    }
    void setHorizontalAlignment(float alignment) {
      m_messageView.setAlignment(alignment, k_verticalAlignment);
    }
    void setMessage(I18n::Message message) {
      m_messageView.setMessage(message);
    }

   private:
    /* Text is placed at the very top of the cell to simplify text centering
     * when there are no cells above. To add a "margin" in other cases, we
     * precede the message row with an empty row. */
    constexpr static float k_verticalAlignment = KDGlyph::k_alignTop;
    int numberOfSubviews() const override { return 1; }
    Escher::View *subviewAtIndex(int index) override {
      assert(index == 0);
      return &m_messageView;
    }
    void layoutSubviews(bool force = false) override {
      setChildFrame(&m_messageView, bounds(), force);
    }
    bool protectedIsSelectable() override { return false; }
    Escher::MessageTextView m_messageView;
  };

  class EmptyCell : public Escher::HighlightCell {
   public:
    void drawRect(KDContext *ctx, KDRect rect) const override {
      ctx->fillRect(bounds(),
                    SolutionsController::ContentView::k_backgroundColor);
    }
  };

  // Cell types
  constexpr static int k_symbolCellType = 0;
  constexpr static int k_deltaCellType = 1;
  constexpr static int k_exactValueCellType = 2;
  constexpr static int k_approximateValueCellType = 3;
  constexpr static int k_messageCellType = 4;
  constexpr static int k_emptyCellType = 5;

  // Fonts
  constexpr static KDFont::Size k_deltaFont = KDFont::Size::Small;
  constexpr static KDFont::Size k_solutionsFont = KDFont::Size::Large;

  // Heights and widths
  constexpr static KDCoordinate k_defaultCellHeight = 20;
  /* We concatenate symbol name with a number of at most 2 digits.
   * The quotation marks are removed if the name has some, so that the
   * cell is not too wide. */
  constexpr static int k_symbolCellWidth =
      KDFont::GlyphWidth(k_solutionsFont) *
          (Poincare::SymbolAbstractNode::k_maxNameLengthWithoutQuotationMarks +
           2) +
      2 * Escher::AbstractEvenOddBufferTextCell::k_horizontalMargin;
  constexpr static int k_valueCellWidth =
      Ion::Display::Width - k_symbolCellWidth -
      Escher::Metric::CommonLeftMargin - Escher::Metric::CommonRightMargin;

  /* Number of cells
   * When displaying approximate solutions for cos(x) = 0 between 0 and 1800 and
   * scrolling down */
  constexpr static int k_maxNumberOfVisibleCells =
      (Ion::Display::Height - 3 * Escher::Metric::TitleBarHeight) /
          k_defaultCellHeight +
      1;
  // This assert is just for information purposes
  static_assert(k_maxNumberOfVisibleCells == 10,
                "k_maxNumberOfVisibleCells has changed");
  static_assert(
      k_maxNumberOfVisibleCells <=
          SystemOfEquations::k_maxNumberOfSolutions +
              Poincare::Expression::k_maxNumberOfVariables,
      "We can reduce the number of cells in Solver:SolutionsController.");
  constexpr static int k_maxNumberOfSymbols =
      SystemOfEquations::k_maxNumberOfSolutions +
      Poincare::Expression::k_maxNumberOfVariables;
  constexpr static int k_numberOfSymbolCells =
      (k_maxNumberOfVisibleCells < k_maxNumberOfSymbols)
          ? k_maxNumberOfVisibleCells
          : k_maxNumberOfSymbols;
  constexpr static int k_maxNumberOfExactValues =
      SystemOfEquations::k_maxNumberOfExactSolutions +
      Poincare::Expression::k_maxNumberOfVariables;
  constexpr static int k_numberOfExactValueCells =
      (k_maxNumberOfVisibleCells < k_maxNumberOfExactValues)
          ? k_maxNumberOfVisibleCells
          : k_maxNumberOfExactValues;
  constexpr static int k_numberOfApproximateValueCells =
      1 + (k_maxNumberOfVisibleCells <
                   SystemOfEquations::k_maxNumberOfApproximateSolutions
               ? k_maxNumberOfVisibleCells
               : SystemOfEquations::k_maxNumberOfApproximateSolutions);
  constexpr static int k_numberOfMessageCells = 2;
  constexpr static int k_numberOfEmptyCells = 2;

  bool usedUserVariables() const;
  int userVariablesMessageRow() const;
  I18n::Message noSolutionMessage();

  Escher::SmallFontEvenOddBufferTextCell m_symbolCells[k_numberOfSymbolCells];
  Escher::EvenOddExpressionCell m_deltaCell;
  Poincare::Layout m_delta2Layout;
  Poincare::Layout m_delta3Layout;
  Shared::ScrollableTwoLayoutsCell m_exactValueCells[k_numberOfExactValueCells];
  Escher::FloatEvenOddBufferTextCell<>
      m_approximateValueCells[k_numberOfApproximateValueCells];
  MessageCell m_messageCells[k_numberOfMessageCells];
  EmptyCell m_emptyCell[k_numberOfEmptyCells];
  ContentView m_contentView;
};

}  // namespace Solver

#endif
