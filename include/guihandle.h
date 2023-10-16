#include <Wt/WComboBox.h>
#include <Wt/WGridLayout.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WLayout.h>
#include <Wt/WPushButton.h>
#include <Wt/WServer.h>
#include <Wt/WSlider.h>
#include <Wt/WTable.h>
#include <Wt/WText.h>
#include <Wt/WVBoxLayout.h>

#include <iostream>
#include <string>
#include <vector>

#include "conf.h"

// This is a header abstraction for adding some of the widgets.
// This significantly cleans up the code in the main server file.
namespace GUI {
// T is for table, F is for function and I is for class instance.
template <typename T, typename F, typename I>
// Function that adds a slider to a table.
Wt::WSlider *addSliderToTable(const T &table, int x, int y, int min, int max,
                              int tickInt, int val, F fun, I inst) {
    auto slider = table->elementAt(x, y)->addWidget(
        std::make_unique<Wt::WSlider>(Wt::Orientation::Horizontal));
    slider->resize(400, 30);
    slider->setMinimum(min);
    slider->setMaximum(max);
    slider->setValue(val);
    slider->setTickInterval(tickInt);
    slider->setTickPosition(Wt::WSlider::TicksBothSides);
    slider->valueChanged().connect(inst, fun);
    return slider;
}

// T is for layout, F is for function and I is for class instance.
template <typename T, typename F, typename I>
// Function that adds a combobox with items.
Wt::WComboBox *addComboBox(const T &layout, std::vector<std::string> items,
                           F fun, I inst) {
    auto combo = layout->addWidget(std::make_unique<Wt::WComboBox>());
    combo->resize(Wt::WLength::Auto, COMBO_SIZE);
    combo->decorationStyle().font().setSize(Wt::FontSize::XXLarge);
    for (auto &el : items) combo->addItem(el);
    combo->changed().connect(inst, fun);
    return combo;
}

// T is for table, F is for function and I is for class instance.
template <typename T, typename F, typename I>
// Function that adds a combobox with items to a table.
Wt::WComboBox *addComboBox(const T &layout, std::vector<std::string> items,
                           int x, int y, F fun, I inst) {
    auto combo =
        layout->elementAt(x, y)->addWidget(std::make_unique<Wt::WComboBox>());
    combo->resize(Wt::WLength::Auto, COMBO_SIZE_SMALLER);
    combo->decorationStyle().font().setSize(Wt::FontSize::Large);
    for (auto &el : items) combo->addItem(el);
    combo->changed().connect(inst, fun);
    return combo;
}

// T is for layout, F is for function and I is for class instance.
template <typename T, typename F, typename I>
// Function that adds a combobox.
Wt::WComboBox *addComboBox(const T &layout, F fun, I inst) {
    auto combo = layout->addWidget(std::make_unique<Wt::WComboBox>());
    combo->resize(Wt::WLength::Auto, COMBO_SIZE);
    combo->decorationStyle().font().setSize(Wt::FontSize::XXLarge);
    combo->changed().connect(inst, fun);
    return combo;
}
}  // namespace GUI