#pragma once
#include <array>

// Held MIDI notes in press order (last = most recent), allocation-free for the audio thread.
// A note is stored at most once, so 128 slots always suffice.
struct NoteStack
{
    void clear() noexcept                  { count = 0; }
    bool empty() const noexcept            { return count == 0; }
    int  size() const noexcept             { return count; }
    int  back() const noexcept             { return count > 0 ? notes[(size_t) (count - 1)] : -1; }
    void remove (int note) noexcept
    {
        int w = 0;
        for (int r = 0; r < count; ++r)
            if (notes[(size_t) r] != note) notes[(size_t) w++] = notes[(size_t) r];
        count = w;
    }
    void push (int note) noexcept          // moves an already-held note to the top
    {
        remove (note);
        if (count < (int) notes.size()) notes[(size_t) count++] = note;
    }

private:
    std::array<int, 128> notes {};
    int count = 0;
};
