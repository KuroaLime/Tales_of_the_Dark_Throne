#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

// Geometry only: the caller supplies decoded top-down pixels and caches the
// result with its bitmap. No pixels or source assets are modified here.
namespace CharacterSpriteLayout
{
    namespace WalkCycle
    {
        constexpr int Columns = 4;
        constexpr int Rows = 3;
        constexpr int FrameCount = Columns * Rows;
        constexpr int CellSize = 96;
        constexpr float RootX = 48.0f;
        constexpr float GroundY = 88.0f;
    }

    namespace IdlePose
    {
        constexpr int Columns = 1;
        constexpr int Rows = 1;
        constexpr int FrameCount = 1;
        constexpr int CellSize = WalkCycle::CellSize;
        constexpr float RootX = WalkCycle::RootX;
        constexpr float GroundY = WalkCycle::GroundY;

        constexpr bool HasDimensions(int width, int height)
        {
            return width == CellSize && height == CellSize;
        }
    }

    namespace IdleCycle
    {
        constexpr int Columns = WalkCycle::Columns;
        constexpr int Rows = WalkCycle::Rows;
        constexpr int FrameCount = Columns * Rows;
        constexpr int CellSize = WalkCycle::CellSize;
        constexpr float RootX = WalkCycle::RootX;
        constexpr float GroundY = WalkCycle::GroundY;
        constexpr bool HasDimensions(int width, int height)
        {
            return width == Columns * CellSize && height == Rows * CellSize;
        }
    }

    namespace Action
    {
        constexpr int Columns = 4;
        constexpr int Rows = 3;
        constexpr int FrameCount = Columns * Rows;
        constexpr int CellSize = 128;
        constexpr float RootX = 64.0f;
        constexpr float GroundY = 112.0f;
        // Extra cell padding accommodates weapons and collapsed poses without
        // shrinking the character relative to the 96px idle/walk reference.
        constexpr int ReferenceFrameHeight = WalkCycle::CellSize;

        constexpr bool HasDimensions(int width, int height)
        {
            return width == Columns * CellSize && height == Rows * CellSize;
        }
    }

    // WalkStop (two six-pose branches) and IdleReturn (three four-pose rows)
    // use the same padded cells and body scale as authored combat actions.
    namespace IdleTransition
    {
        constexpr int Columns = Action::Columns;
        constexpr int Rows = Action::Rows;
        constexpr int CellSize = Action::CellSize;
        constexpr int FrameCount = Action::FrameCount;
        constexpr float RootX = Action::RootX;
        constexpr float GroundY = Action::GroundY;
        constexpr int ReferenceFrameHeight = Action::ReferenceFrameHeight;
        constexpr bool HasDimensions(int width, int height) { return Action::HasDimensions(width, height); }
    }

    struct Rect
    {
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
    };

    struct Frame
    {
        Rect source;
        float anchorX = 0.0f;
        float anchorY = 0.0f;
    };

    constexpr Frame WithWalkCycleAnchor(Frame frame, int index)
    {
        frame.anchorX = (index % WalkCycle::Columns) * WalkCycle::CellSize + WalkCycle::RootX;
        frame.anchorY = (index / WalkCycle::Columns) * WalkCycle::CellSize + WalkCycle::GroundY;
        return frame;
    }

    constexpr Frame WithIdlePoseAnchor(Frame frame)
    {
        frame.anchorX = IdlePose::RootX;
        frame.anchorY = IdlePose::GroundY;
        return frame;
    }

    constexpr Frame WithActionAnchor(Frame frame, int index)
    {
        frame.anchorX = (index % Action::Columns) * Action::CellSize + Action::RootX;
        frame.anchorY = (index / Action::Columns) * Action::CellSize + Action::GroundY;
        return frame;
    }

    struct Layout
    {
        int nominalFrameWidth = 0;
        int nominalFrameHeight = 0;
        std::vector<Frame> frames;
    };

    struct DrawRect
    {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    namespace Detail
    {
        constexpr bool IsOpaque(std::uint32_t pixel)
        {
            // Match TransparentBlt's RGB key; bitmap alpha is irrelevant.
            return (pixel & 0x00ffffffu) != 0x00ff00ffu;
        }

        constexpr bool CanRoundPixel(float value)
        {
            return value == value &&
                value > static_cast<float>((std::numeric_limits<int>::min)()) &&
                value < static_cast<float>((std::numeric_limits<int>::max)());
        }

        constexpr int RoundPixel(float value)
        {
            // Round half away from zero, including negative anchor offsets.
            return static_cast<int>(value >= 0.0f ? value + 0.5f : value - 0.5f);
        }

        inline int UniformBoundary(int extent, int index, int count)
        {
            return static_cast<int>(static_cast<std::int64_t>(extent) * index / count);
        }

        inline std::vector<int> FindCuts(const std::vector<int>& coverage, int count)
        {
            const int extent = static_cast<int>(coverage.size());
            const int radius = (extent / count) / 5;
            std::vector<int> cuts(static_cast<std::size_t>(count) + 1);
            cuts.front() = 0;
            cuts.back() = extent;
            for (int index = 1; index < count; ++index)
            {
                const int nominal = UniformBoundary(extent, index, count);
                const int begin = (std::max)(cuts[index - 1] + 1, nominal - radius);
                // Leave at least one pixel for each subsequent cell.
                const int end = (std::min)(extent - (count - index) + 1,
                    nominal + radius + 1);
                int best = nominal;
                int bestDistance = (std::numeric_limits<int>::max)();
                for (int position = begin; position < end;)
                {
                    if (coverage[position] != 0)
                    {
                        ++position;
                        continue;
                    }
                    const int runBegin = position;
                    while (position < end && coverage[position] == 0) ++position;
                    const int candidate = runBegin + (position - runBegin) / 2;
                    const int distance = candidate > nominal ? candidate - nominal : nominal - candidate;
                    if (distance < bestDistance)
                    {
                        best = candidate;
                        bestDistance = distance;
                    }
                }
                // If effects bridge the gutter, retain the uniform separator.
                cuts[index] = (std::max)(cuts[index - 1] + 1,
                    (std::min)(extent - (count - index), best));
            }
            return cuts;
        }

        inline float FootMedianX(const std::uint32_t* pixels, int width, const Rect& content)
        {
            const int height = content.bottom - content.top;
            const int bandHeight = (std::max)(1,
                static_cast<int>((static_cast<std::int64_t>(height) * 15 + 99) / 100));
            const int bandTop = content.bottom - bandHeight;
            std::vector<int> coverage(static_cast<std::size_t>(content.right - content.left), 0);
            std::uint64_t total = 0;
            for (int y = bandTop; y < content.bottom; ++y)
            {
                const std::size_t row = static_cast<std::size_t>(y) * width;
                for (int x = content.left; x < content.right; ++x)
                {
                    if (!IsOpaque(pixels[row + x])) continue;
                    ++coverage[x - content.left];
                    ++total;
                }
            }
            if (total == 0) return content.left + (content.right - content.left) * 0.5f;

            const std::uint64_t lowerIndex = (total - 1) / 2;
            const std::uint64_t upperIndex = total / 2;
            std::uint64_t cumulative = 0;
            int lower = content.left;
            int upper = content.left;
            bool haveLower = false;
            for (int x = content.left; x < content.right; ++x)
            {
                cumulative += coverage[x - content.left];
                if (!haveLower && cumulative > lowerIndex)
                {
                    lower = x;
                    haveLower = true;
                }
                if (cumulative > upperIndex)
                {
                    upper = x;
                    break;
                }
            }
            // Median of pixel centers, including the mean of two central pixels.
            return lower + (upper - lower + 1) * 0.5f;
        }
    }

    // Uniform source-to-screen scale: cropping never changes the body's size.
    // The source rectangle is mirrored around its own bounds when flipped;
    // its ground anchor therefore uses the distance from the opposite edge.
    constexpr DrawRect PlaceAtFoot(const Frame& frame, int nominalFrameHeight,
        int referenceFrameHeight, int footX, int footY, bool flipped)
    {
        const std::int64_t sourceWidth = static_cast<std::int64_t>(frame.source.right) - frame.source.left;
        const std::int64_t sourceHeight = static_cast<std::int64_t>(frame.source.bottom) - frame.source.top;
        if (nominalFrameHeight <= 0 || referenceFrameHeight <= 0 ||
            sourceWidth <= 0 || sourceHeight <= 0)
        {
            return DrawRect{};
        }
        const float scale = static_cast<float>(referenceFrameHeight) / nominalFrameHeight;
        const float scaledWidth = static_cast<float>(sourceWidth) * scale;
        const float scaledHeight = static_cast<float>(sourceHeight) * scale;
        const float offsetX = (flipped ? frame.source.right - frame.anchorX :
            frame.anchorX - frame.source.left) * scale;
        const float offsetY = (frame.anchorY - frame.source.top) * scale;
        if (!Detail::CanRoundPixel(scaledWidth) || !Detail::CanRoundPixel(scaledHeight) ||
            !Detail::CanRoundPixel(offsetX) || !Detail::CanRoundPixel(offsetY))
        {
            return DrawRect{};
        }
        const int drawWidth = Detail::RoundPixel(scaledWidth);
        const int drawHeight = Detail::RoundPixel(scaledHeight);
        const std::int64_t drawX = static_cast<std::int64_t>(footX) - Detail::RoundPixel(offsetX);
        const std::int64_t drawY = static_cast<std::int64_t>(footY) - Detail::RoundPixel(offsetY);
        if (drawWidth <= 0 || drawHeight <= 0 ||
            drawX < (std::numeric_limits<int>::min)() || drawX > (std::numeric_limits<int>::max)() ||
            drawY < (std::numeric_limits<int>::min)() || drawY > (std::numeric_limits<int>::max)())
        {
            return DrawRect{};
        }
        return DrawRect{ static_cast<int>(drawX), static_cast<int>(drawY), drawWidth, drawHeight };
    }

    inline Layout Analyze(const std::uint32_t* topDownArgb, int width, int height,
        int columns, int rows, bool alignFeet)
    {
        Layout layout;
        if (topDownArgb == nullptr || width <= 0 || height <= 0 ||
            columns <= 0 || rows <= 0 || columns > width || rows > height)
        {
            return layout;
        }
        // Prevent overflow in indexing or frame-count calculation. The caller
        // remains responsible for supplying width * height readable pixels.
        const std::size_t maxSize = (std::numeric_limits<std::size_t>::max)();
        if (static_cast<std::size_t>(width) > maxSize / static_cast<std::size_t>(height) ||
            static_cast<std::size_t>(columns) > maxSize / static_cast<std::size_t>(rows))
        {
            return layout;
        }

        layout.nominalFrameWidth = width / columns;
        layout.nominalFrameHeight = height / rows;
        std::vector<int> rowCoverage(static_cast<std::size_t>(height), 0);
        for (int y = 0; y < height; ++y)
        {
            const std::size_t row = static_cast<std::size_t>(y) * width;
            for (int x = 0; x < width; ++x)
                if (Detail::IsOpaque(topDownArgb[row + x])) ++rowCoverage[y];
        }
        const std::vector<int> rowCuts = Detail::FindCuts(rowCoverage, rows);
        layout.frames.reserve(static_cast<std::size_t>(columns) * rows);
        for (int rowIndex = 0; rowIndex < rows; ++rowIndex)
        {
            const int top = rowCuts[rowIndex];
            const int bottom = rowCuts[rowIndex + 1];
            std::vector<int> columnCoverage(static_cast<std::size_t>(width), 0);
            for (int y = top; y < bottom; ++y)
            {
                const std::size_t row = static_cast<std::size_t>(y) * width;
                for (int x = 0; x < width; ++x)
                    if (Detail::IsOpaque(topDownArgb[row + x])) ++columnCoverage[x];
            }
            const std::vector<int> columnCuts = Detail::FindCuts(columnCoverage, columns);
            for (int column = 0; column < columns; ++column)
            {
                const int left = columnCuts[column];
                const int right = columnCuts[column + 1];
                Rect content{ right, bottom, left, top };
                for (int y = top; y < bottom; ++y)
                {
                    const std::size_t row = static_cast<std::size_t>(y) * width;
                    for (int x = left; x < right; ++x)
                    {
                        if (!Detail::IsOpaque(topDownArgb[row + x])) continue;
                        content.left = (std::min)(content.left, x);
                        content.top = (std::min)(content.top, y);
                        content.right = (std::max)(content.right, x + 1);
                        content.bottom = (std::max)(content.bottom, y + 1);
                    }
                }

                Frame frame;
                frame.anchorX = column * layout.nominalFrameWidth + layout.nominalFrameWidth * 0.5f;
                frame.anchorY = static_cast<float>((rowIndex + 1) * layout.nominalFrameHeight);
                if (content.left >= content.right || content.top >= content.bottom)
                {
                    // A fully transparent cell stays transparent with a valid
                    // source rectangle and the same nominal anchor as before.
                    frame.source = Rect{ left, top, right, bottom };
                }
                else
                {
                    frame.source = content;
                    if (alignFeet)
                    {
                        frame.anchorX = Detail::FootMedianX(topDownArgb, width, content);
                        frame.anchorY = static_cast<float>(content.bottom);
                    }
                }
                layout.frames.push_back(frame);
            }
        }
        return layout;
    }
}
