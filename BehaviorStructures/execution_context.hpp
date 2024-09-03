#pragma once

namespace fluczakAI
{
/**
 * \brief A base class for an execution context- it is used to store useful data while
 * executing AI behavior selection structures
 */
struct AIExecutionContext
    {
        virtual ~AIExecutionContext() = default;
    };
}  // namespace bee::ai
