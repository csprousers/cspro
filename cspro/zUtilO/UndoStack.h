#pragma once


template<typename T>
class UndoStack
{
    static_assert(!std::is_pointer_v<T>, "use a smart pointer instead");

public:
    UndoStack(size_t max_stack_size = 12)
        :   m_maxStackSize(max_stack_size)
    {
        ASSERT(m_maxStackSize > 0);
    }

    bool CanUndo() const { return !m_undoStack.empty(); }
    bool CanRedo() const { return !m_redoStack.empty(); }

    void ClearUndo() { m_undoStack.clear(); }
    void ClearRedo() { m_redoStack.clear(); }
    void Clear()     { ClearUndo(); ClearRedo(); }

    void PushUndo(T undo_value) { Push(m_undoStack, std::move(undo_value)); }
    void PushRedo(T redo_value) { Push(m_redoStack, std::move(redo_value)); }

    T Undo() { return Get(m_undoStack); }
    T Redo() { return Get(m_redoStack); }

private:
    void Push(std::vector<T>& stack, T value)
    {
        // remove old entries
        while( stack.size() >= m_maxStackSize )
            stack.erase(stack.begin());

        stack.emplace_back(std::move(value));
    }

    T Get(std::vector<T>& stack)
    {
        ASSERT(!stack.empty());

        T value = std::move(stack.back());
        stack.pop_back();

        return value;
    }

private:
    std::vector<T> m_undoStack;
    std::vector<T> m_redoStack;
    size_t m_maxStackSize;
};
