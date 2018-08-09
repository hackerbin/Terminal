/*++
Copyright (c) Microsoft Corporation

Module Name:
- OutputCellView.hpp

Abstract:
- Read-only view into a single cell of data that someone is attempting to write into the output buffer.
- This is done for performance reasons (avoid heap allocs and copies).

Author:
- Michael Niksa (MiNiksa) 06-Oct-2018

Revision History:
- Based on work from OutputCell.hpp/cpp by Austin Diviness (AustDi)

--*/

#pragma once

#include "DbcsAttribute.hpp"
#include "TextAttribute.hpp"
#include "OutputCell.hpp"

class OutputCellView
{
public:

    OutputCellView(const std::wstring_view view,
                   const DbcsAttribute dbcsAttr,
                   const TextAttribute textAttr,
                   const OutputCell::TextAttributeBehavior behavior);

    std::wstring_view Chars() const;
    DbcsAttribute DbcsAttr() const;
    TextAttribute TextAttr() const;
    OutputCell::TextAttributeBehavior TextAttrBehavior() const;

    bool operator==(const OutputCellView& view) const noexcept;
    bool operator!=(const OutputCellView& view) const noexcept;

private:
    std::wstring_view _view;
    DbcsAttribute _dbcsAttr;
    TextAttribute _textAttr;
    OutputCell::TextAttributeBehavior _behavior;
};
