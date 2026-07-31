#include "EditorCommandHistory.h"

namespace Himii
{
    EditorCommandHistory::EditorCommandHistory(size_t maximumDepth)
        : m_MaximumDepth(maximumDepth)
    {
    }

    void EditorCommandHistory::Execute(Scope<IEditorCommand> command)
    {
        if (!command)
            return;

        command->Execute();

        if (!m_UndoStack.empty())
        {
            if (m_UndoStack.back()->TryMerge(*command))
            {
                m_RedoStack.clear();
                return;
            }
        }

        m_UndoStack.push_back(std::move(command));
        m_RedoStack.clear();

        if (m_UndoStack.size() > m_MaximumDepth)
            m_UndoStack.erase(m_UndoStack.begin());
    }

    bool EditorCommandHistory::Undo()
    {
        if (m_UndoStack.empty())
            return false;

        auto command = std::move(m_UndoStack.back());
        m_UndoStack.pop_back();
        command->Undo();
        m_RedoStack.push_back(std::move(command));
        return true;
    }

    bool EditorCommandHistory::Redo()
    {
        if (m_RedoStack.empty())
            return false;

        auto command = std::move(m_RedoStack.back());
        m_RedoStack.pop_back();
        command->Execute();
        m_UndoStack.push_back(std::move(command));
        return true;
    }

    void EditorCommandHistory::Clear()
    {
        m_UndoStack.clear();
        m_RedoStack.clear();
    }
}
