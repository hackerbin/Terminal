// Windows Internal Libraries (wil)
// TraceLogging.h: Helpers for managing tracelogging, provider handles, telemetry, and activities
//
// Usage Guidelines:
// https://microsoft.sharepoint.com/teams/osg_development/Shared%20Documents/Windows%20TraceLogging%20Helpers.docx?web=1
//
// wil Usage Guidelines:
// https://microsoft.sharepoint.com/teams/osg_development/Shared%20Documents/Windows%20Internal%20Libraries%20for%20C++%20Usage%20Guide.docx?web=1
//
// wil Discussion Alias (wildisc):
// http://idwebelements/GroupManagement.aspx?Group=wildisc&Operation=join  (one-click join)

#pragma once

// Note that we avoid pulling in STL's memory header from TraceLogging.h through Resource.h as we have
// TraceLogging customers who are still on older versions of STL (without std::shared_ptr<>).
#define RESOURCE_SUPPRESS_STL
#include "Result.h"
#undef RESOURCE_SUPPRESS_STL
#include <winmeta.h>            // WINEVENT_KEYWORD_TELEMETRY
#include <TraceLoggingProvider.h>
#include <TraceLoggingActivity.h>
#include <telemetry\microsofttelemetry.h>
#ifndef TRACELOGGING_SUPPRESS_NEW
#include <new>
#endif

#pragma warning(push)
#pragma warning(disable: 26135)   // Missing locking annotation, Caller failing to hold lock

#ifndef __TRACELOGGING_TEST_HOOK_ERROR
#define __TRACELOGGING_TEST_HOOK_ERROR(failure)
#define __TRACELOGGING_TEST_HOOK_ACTIVITY_ERROR(failure)
#define __TRACELOGGING_TEST_HOOK_CALLCONTEXT_ERROR(pFailure, hr)
#define __TRACELOGGING_TEST_HOOK_ACTIVITY_START()
#define __TRACELOGGING_TEST_HOOK_ACTIVITY_STOP(pFailure, hr)
#define __TRACELOGGING_TEST_HOOK_SET_ENABLED false
#define __TRACELOGGING_TEST_HOOK_VERIFY_API_TELEMETRY(nameSpace, apiList, specializationList, countArray, numCounters) 
#define __TRACELOGGING_TEST_HOOK_API_TELEMETRY_EVENT_DELAY_MS 5000
#endif

/*
Macro __TRACELOGGING_DEFINE_PROVIDER_STORAGE_LINK(name1, name2):
This macro defines a storage link association between two names for use by the
TlgReflector static analysis tool.
*/
#define __TRACELOGGING_DEFINE_PROVIDER_STORAGE_LINK(name1, name2) \
    __annotation(L"_TlgProviderLink:|" _TLG_PASTE(L, _TLG_STRINGIZE(__LINE__)) L"|Key|" _TLG_PASTE(L, _TLG_STRINGIZE(name1)) L"=" _TLG_PASTE(L, _TLG_STRINGIZE(name2)))

// Utility macro for writing relevant fields from a wil::FailureInfo structure into a TraceLoggingWrite
// statement.  Most fields are relevant for telemetry or for simple ETW, but there are a few additional
// fields reported via ETW.

#define __RESULT_TELEMETRY_COMMON_FAILURE_PARAMS(failure) \
    TraceLoggingUInt32((failure).hr, "hresult"), \
    TraceLoggingString((failure).pszFile, "fileName"), \
    TraceLoggingUInt32((failure).uLineNumber, "lineNumber"), \
    TraceLoggingString((failure).pszModule, "module"), \
    TraceLoggingUInt32(static_cast<DWORD>((failure).type), "failureType"), \
    TraceLoggingWideString((failure).pszMessage, "message"), \
    TraceLoggingUInt32((failure).threadId, "threadId"), \
    TraceLoggingString((failure).pszCallContext, "callContext"), \
    TraceLoggingUInt32((failure).callContextOriginating.contextId, "originatingContextId"), \
    TraceLoggingString((failure).callContextOriginating.contextName, "originatingContextName"), \
    TraceLoggingWideString((failure).callContextOriginating.contextMessage, "originatingContextMessage"), \
    TraceLoggingUInt32((failure).callContextCurrent.contextId, "currentContextId"), \
    TraceLoggingString((failure).callContextCurrent.contextName, "currentContextName"), \
    TraceLoggingWideString((failure).callContextCurrent.contextMessage, "currentContextMessage")

#define __RESULT_TRACELOGGING_COMMON_FAILURE_PARAMS(failure) \
    __RESULT_TELEMETRY_COMMON_FAILURE_PARAMS(failure), \
    TraceLoggingUInt32(static_cast<DWORD>((failure).failureId), "failureId"), \
    TraceLoggingUInt32(static_cast<DWORD>((failure).cFailureCount), "failureCount"), \
    TraceLoggingString((failure).pszFunction, "function")

// Activity Start Event (ALL)
#define __ACTIVITY_START_PARAMS() \
    TraceLoggingStruct(1, "wilActivity"), \
    TraceLoggingUInt32(::GetCurrentThreadId(), "threadId")

// Activity Stop Event (SUCCESSFUL or those WITHOUT full failure info -- just hr)
// Also utilized for intermediate stop events (a successful call to 'Stop()' from a Split activity
#define __ACTIVITY_STOP_PARAMS(hr) \
    TraceLoggingStruct(2, "wilActivity"), \
    TraceLoggingUInt32(hr, "hresult"), \
    TraceLoggingUInt32(::GetCurrentThreadId(), "threadId") 

// Activity Stop Event (FAILED with full failure info)
#define __ACTIVITY_STOP_TELEMETRY_FAILURE_PARAMS(failure) \
    TraceLoggingStruct(14, "wilActivity"), \
    __RESULT_TELEMETRY_COMMON_FAILURE_PARAMS(failure)
#define __ACTIVITY_STOP_TRACELOGGING_FAILURE_PARAMS(failure) \
    TraceLoggingStruct(17, "wilActivity"), \
    __RESULT_TRACELOGGING_COMMON_FAILURE_PARAMS(failure)

// Activity call to 'ContinueOnThread()'
#define __ACTIVITY_CONTINUE_PARAMS() \
    TraceLoggingStruct(1, "wilActivity"), \
    TraceLoggingUInt32(::GetCurrentThreadId(), "threadId") 

// "ActivityError" tagged event (all distinct FAILURES occurring within the outer activity scope)
#define __ACTIVITY_ERROR_TELEMETRY_FAILURE_PARAMS(failure) \
    TraceLoggingStruct(14, "wilActivity"), \
    __RESULT_TELEMETRY_COMMON_FAILURE_PARAMS(failure)
#define __ACTIVITY_ERROR_TRACELOGGING_FAILURE_PARAMS(failure) \
    TraceLoggingStruct(17, "wilActivity"), \
    __RESULT_TRACELOGGING_COMMON_FAILURE_PARAMS(failure)

// "ActivityFailure" tagged event (only comes through on TELEMETRY for CallContext activities that have FAILED)
#define __ACTIVITY_FAILURE_TELEMETRY_FAILURE_PARAMS(failure) \
    TraceLoggingStruct(14, "wilActivity"), \
    __RESULT_TELEMETRY_COMMON_FAILURE_PARAMS(failure)
#define __ACTIVITY_FAILURE_TELEMETRY_PARAMS(hr, contextName, contextMessage) \
    TraceLoggingStruct(4, "wilActivity"), \
    TraceLoggingUInt32(hr, "hresult"), \
    TraceLoggingUInt32(::GetCurrentThreadId(), "threadId"), \
    TraceLoggingString(contextName, "currentContextName"), \
    TraceLoggingWideString(contextMessage, "currentContextMessage")

// "FallbackError" events (all FAILURE events happening outside of ANY activity context)
#define __RESULT_TELEMETRY_FAILURE_PARAMS(failure) \
    TraceLoggingStruct(14, "wilResult"), \
    __RESULT_TELEMETRY_COMMON_FAILURE_PARAMS(failure)
#define __RESULT_TRACELOGGING_FAILURE_PARAMS(failure) \
    TraceLoggingStruct(17, "wilResult"), \
    __RESULT_TRACELOGGING_COMMON_FAILURE_PARAMS(failure)

namespace wil
{
enum class ActivityOptions
{
    None = 0,
    TelemetryOnFailure = 0x1
};
DEFINE_ENUM_FLAG_OPERATORS(ActivityOptions)

template <typename ActivityTraceLoggingType, 
          ActivityOptions options, UINT64 keyword, UINT8 level,
          typename TlgReflectorTag>
class ActivityBase;

/// @cond
namespace details
{
    // Lazy static initialization helper for holding a singleton telemetry class to maintain
    // the provider handle.

    template<class T>
    class static_lazy
    {
    public:
        ~static_lazy() WI_NOEXCEPT
        {
            void* pVoid;
            BOOL pending;

            // If object is being constructed on another thread, wait until construction completes.
            // Need a memory barrier here (see get() and ~Completer below) so use the result that we
            // get from InitOnceBeginInitialize(..., &pVoid, ...)
            if (::InitOnceBeginInitialize(&m_initOnce, INIT_ONCE_CHECK_ONLY, &pending, &pVoid) && !pending)
            {
                static_cast<T*>(pVoid)->~T();
            }
        }

        T* get() WI_NOEXCEPT
        {
            void* pVoid;
            BOOL pending;

            ::InitOnceBeginInitialize(&m_initOnce, 0, &pending, &pVoid);
            if (pending)
            {
                // Don't do anything non-trivial from DllMain, fail fast.
                // Some 3rd party code in IE calls shell functions this way, so we can only enforce
                // this in DEBUG.
#ifdef DEBUG
                FAIL_FAST_IMMEDIATE_IF_IN_LOADER_CALLOUT();
#endif

                Completer completer(this);
                pVoid = &m_storage;
                ::new(pVoid)T();
                completer.Succeed();
            }
            return static_cast<T*>(pVoid);
        }

    private:
        INIT_ONCE m_initOnce;
        BYTE m_storage[sizeof(T)];
        struct Completer
        {
            static_lazy *m_pSelf;
            DWORD m_flags;

            explicit Completer(static_lazy *pSelf) WI_NOEXCEPT : m_pSelf(pSelf), m_flags(INIT_ONCE_INIT_FAILED) { }
            void Succeed() WI_NOEXCEPT { m_flags = 0; }

            ~Completer() WI_NOEXCEPT
            {
                if (m_flags == 0)
                {
                    reinterpret_cast<T*>(&m_pSelf->m_storage)->Create();
                }
                ::InitOnceComplete(&m_pSelf->m_initOnce, m_flags, &m_pSelf->m_storage);
            }
        };
    };

    // This class serves as a simple RAII wrapper around CallContextInfo.  It presumes that
    // the contextName parameter is always a static string, but copies or allocates the
    // contextMessage as needed.

    class StoredCallContextInfo : public wil::CallContextInfo
    {
    public:
        StoredCallContextInfo()
        {
            ::ZeroMemory(this, sizeof(*this));
        }

        StoredCallContextInfo(StoredCallContextInfo &&other) :
            StoredCallContextInfo()
        {
            operator=(wistd::move(other));
        }

        StoredCallContextInfo& operator=(StoredCallContextInfo &&other)
        {
            contextId = other.contextId;
            contextName = other.contextName;
            auto tempMessage = contextMessage;
            contextMessage = other.contextMessage;
            other.contextMessage = tempMessage;
            auto tempOwns = m_ownsMessage;
            m_ownsMessage = other.m_ownsMessage;
            other.m_ownsMessage = tempOwns;
            return *this;
        }

        StoredCallContextInfo(StoredCallContextInfo const &other) :
            m_ownsMessage(false)
        {
            contextId = other.contextId;
            contextName = other.contextName;
            if (other.m_ownsMessage)
            {
                AssignMessage(other.contextMessage);
            }
            else
            {
                contextMessage = other.contextMessage;
            }
        }

        StoredCallContextInfo(_In_opt_ PCSTR staticContextName) :
            m_ownsMessage(false)
        {
            contextId = 0;
            contextName = staticContextName;
            contextMessage = nullptr;
        }

        StoredCallContextInfo(PCSTR staticContextName, _Printf_format_string_ PCSTR formatString, va_list argList) :
            StoredCallContextInfo(staticContextName)
        {
            SetMessage(formatString, argList);
        }

        void SetMessage(_Printf_format_string_ PCSTR formatString, va_list argList)
        {
            wchar_t loggingMessage[2048];
            PrintLoggingMessage(loggingMessage, ARRAYSIZE(loggingMessage), formatString, argList);
            ClearMessage();
            AssignMessage(loggingMessage);
        }

        void SetMessage(_In_opt_ PCWSTR message)
        {
            ClearMessage();
            contextMessage = message;
        }

        void SetMessageCopy(_In_opt_ PCWSTR message)
        {
            ClearMessage();
            if (message != nullptr)
            {
                AssignMessage(message);
            }
        }

        void ClearMessage()
        {
            if (m_ownsMessage)
            {
                free(const_cast<PWSTR>(contextMessage));
                m_ownsMessage = false;
            }
            contextMessage = nullptr;
        }

        ~StoredCallContextInfo()
        {
            ClearMessage();
        }

        StoredCallContextInfo& operator=(StoredCallContextInfo const &) = delete;

    private:
        void AssignMessage(PCWSTR message)
        {
            auto length = wcslen(message);
            if (length > 0)
            {
                auto sizeBytes = (length + 1) * sizeof(wchar_t);
                contextMessage = static_cast<PCWSTR>(malloc(sizeBytes));
                if (contextMessage != nullptr)
                {
                    m_ownsMessage = true;
                    memcpy_s(const_cast<PWSTR>(contextMessage), sizeBytes, message, sizeBytes);
                }
            }
        }

        bool m_ownsMessage;
    };

    template <typename TActivity>
    void SetRelatedActivityId(TActivity&)
    {
    }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    template <typename ActivityTraceLoggingType, ActivityOptions options, UINT64 keyword, UINT8 level, typename TlgReflectorTag>
    void SetRelatedActivityId(wil::ActivityBase<ActivityTraceLoggingType, options, keyword, level, TlgReflectorTag>& activity)
    {
        GUID capturedRelatedId;
        EventActivityIdControl(EVENT_ACTIVITY_CTRL_GET_ID, &capturedRelatedId);
        activity.SetRelatedActivityId(capturedRelatedId);
    }
#endif

    typedef wistd::integral_constant<char, 0> tag_start;
    typedef wistd::integral_constant<char, 1> tag_start_cv;
} // namespace details
/// @endcond


// This class acts as a simple RAII class returned by a call to ContinueOnCurrentThread() for an activity
// or by a call to WatchCurrentThread() on a provider.  The result is meant to be a stack local variable
// whose scope controls the lifetime of an error watcher on the given thread.  That error watcher re-directs
// errors occurrent within the object's lifetime to the associated provider or activity.
    
class ActivityThreadWatcher
{
public:
    ActivityThreadWatcher() WI_NOEXCEPT
        : m_callbackHolder(nullptr, nullptr, false)
    {}

    ActivityThreadWatcher(_In_ details::IFailureCallback *pCallback, PCSTR staticContextName) WI_NOEXCEPT :
        m_callContext(staticContextName),
        m_callbackHolder(pCallback, &m_callContext)
    {
    }

    ActivityThreadWatcher(_In_ details::IFailureCallback *pCallback, PCSTR staticContextName, _Printf_format_string_ PCSTR formatString, va_list argList) WI_NOEXCEPT :
        ActivityThreadWatcher(pCallback, staticContextName)
    {
        m_callContext.SetMessage(formatString, argList);
    }

    // Uses the supplied StoredCallContextInfo rather than producing one itself
    ActivityThreadWatcher(_In_ details::IFailureCallback *pCallback, _In_ details::StoredCallContextInfo const &callContext) WI_NOEXCEPT :
        m_callContext(callContext),
        m_callbackHolder(pCallback, &m_callContext)
    {
    }

    ActivityThreadWatcher(ActivityThreadWatcher &&other) WI_NOEXCEPT :
        m_callContext(wistd::move(other.m_callContext)),
        m_callbackHolder(wistd::move(other.m_callbackHolder))
    {
        m_callbackHolder.SetCallContext(&m_callContext);
    }

    ActivityThreadWatcher(ActivityThreadWatcher const &) = delete;
    ActivityThreadWatcher& operator=(ActivityThreadWatcher const &) = delete;

    void SetMessage(_Printf_format_string_ PCSTR formatString, ...)
    {
        va_list argList;
        va_start(argList, formatString);
        m_callContext.SetMessage(formatString, argList);
    }

    void SetMessage(_In_opt_ PCWSTR message)
    {
        m_callContext.SetMessage(message);
    }

    void SetMessageCopy(_In_opt_ PCWSTR message)
    {
        m_callContext.SetMessageCopy(message);
    }

private:
    details::StoredCallContextInfo m_callContext;
    details::ThreadFailureCallbackHolder m_callbackHolder;
};


// This is the base-class implementation of a TraceLogging class.  TraceLogging classes are defined with
// BEGIN_TRACELOGGING_CLASS and automatically derive from this class

enum class ErrorReportingType
{
    None = 0,
    Telemetry,
    TraceLogging
};

class TraceLoggingProvider : public details::IFailureCallback
{
public:
    // Only one instance of each of these derived classes should be created
    TraceLoggingProvider(_In_ TraceLoggingProvider const&) = delete;
    TraceLoggingProvider& operator=(TraceLoggingProvider const&) = delete;
    void* operator new(size_t) = delete;
    void* operator new[](size_t) = delete;

protected:

    // This can be overridden to provide specific initialization code for any individual provider.
    // It will be ran once when the single static singleton instance of this class is created.
    virtual void Initialize() WI_NOEXCEPT {}

    // This method can be overriden by a provider to more tightly control what happens in the event
    // of a failure in a CallContext activity, WatchCurrentThread() object, or attributed to a specific failure.
    virtual void OnErrorReported(bool alreadyReported, FailureInfo const &failure) WI_NOEXCEPT
    {
        if (!alreadyReported)
        {
            if (m_errorReportingType == ErrorReportingType::Telemetry)
            {
                ReportTelemetryFailure(failure);
            }
            else if (m_errorReportingType == ErrorReportingType::TraceLogging)
            {
                ReportTraceLoggingFailure(failure);
            }
        }
    }

public:
    TraceLoggingHProvider const Provider_() const WI_NOEXCEPT
    {
        return m_providerHandle;
    }

protected:
    TraceLoggingProvider() WI_NOEXCEPT {}

    virtual ~TraceLoggingProvider() WI_NOEXCEPT
    {
        if (m_ownsProviderHandle)
        {
            TraceLoggingUnregister(m_providerHandle);
        }
    }

    bool IsEnabled_() const WI_NOEXCEPT
    {
        return ((m_providerHandle != nullptr) && TraceLoggingProviderEnabled(m_providerHandle, 0, 0)) || __TRACELOGGING_TEST_HOOK_SET_ENABLED;
    }

    void SetErrorReportingType_(ErrorReportingType type)
    {
        m_errorReportingType = type;
    }

    static bool WasAlreadyReportedToTelemetry(long failureId) WI_NOEXCEPT
    {
        static long volatile s_lastFailureSeen = -1;
        auto wasSeen = (s_lastFailureSeen == failureId);
        s_lastFailureSeen = failureId;
        return wasSeen;
    }

    void ReportTelemetryFailure(FailureInfo const &failure) WI_NOEXCEPT
    {
        __TRACELOGGING_TEST_HOOK_ERROR(failure);
        TraceLoggingWrite(m_providerHandle, "FallbackError", TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY), __RESULT_TELEMETRY_FAILURE_PARAMS(failure));
    }

    void ReportTraceLoggingFailure(FailureInfo const &failure) WI_NOEXCEPT
    {
        TraceLoggingWrite(m_providerHandle, "FallbackError", __RESULT_TRACELOGGING_FAILURE_PARAMS(failure));
    }

    // Helper function for TraceLoggingInfo/TraceLoggingError.
    // It prints out trace message for debug purpose. The message does not go into the telemetry.
    void ReportTraceLoggingMessage(bool isError, _In_ _Printf_format_string_ PCSTR formatString, va_list argList) WI_NOEXCEPT
    {
        if (IsEnabled_())
        {
            wchar_t loggingMessage[2048];
            details::PrintLoggingMessage(loggingMessage, ARRAYSIZE(loggingMessage), formatString, argList);
            if (isError)
            {
                TraceLoggingWrite(m_providerHandle, "TraceLoggingError", TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE), TraceLoggingWideString(loggingMessage, "traceLoggingMessage"));
            }
            else
            {
                TraceLoggingWrite(m_providerHandle, "TraceLoggingInfo", TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE), TraceLoggingWideString(loggingMessage, "traceLoggingMessage"));
            }
        }
    }

    void Register(TraceLoggingHProvider const providerHandle) WI_NOEXCEPT
    {
        // taking over the lifetime and management of providerHandle
        m_providerHandle = providerHandle;
        m_ownsProviderHandle = true;
        TraceLoggingRegister(providerHandle);
        InternalInitialize();
    }

    void AttachProvider(TraceLoggingHProvider const providerHandle) WI_NOEXCEPT
    {
        m_providerHandle = providerHandle;
        m_ownsProviderHandle = false;
        InternalInitialize();
    }

private:
    // IFailureCallback
    bool NotifyFailure(FailureInfo const &failure) WI_NOEXCEPT override
    {
        if (!WasAlreadyReportedToTelemetry(failure.failureId))
        {
            OnErrorReported(false, failure);
        }
        return true;
    }

    void InternalInitialize()
    {
        m_errorReportingType = ErrorReportingType::Telemetry;
        Initialize();
    }

    TraceLoggingHProvider m_providerHandle;
    bool m_ownsProviderHandle;
    ErrorReportingType m_errorReportingType;
};

template<
    typename TraceLoggingType,
    UINT64 keyword = 0,
    UINT8 level = WINEVENT_LEVEL_VERBOSE,
    typename TlgReflectorTag = _TlgReflectorTag_Param0IsProviderType> // helps TlgReflector understand that this is a wrapper type
class BasicActivity
    : public _TlgActivityBase<BasicActivity<TraceLoggingType, keyword, level, TlgReflectorTag>, keyword, level>
{
    typedef
        _TlgActivityBase<BasicActivity<TraceLoggingType, keyword, level, TlgReflectorTag>, keyword, level>
        BaseTy;
    friend class BaseTy;

    void OnStarted()
    {
    }

    void OnStopped()
    {
    }

public:

    BasicActivity()
    {
    }

    BasicActivity(BasicActivity&& rhs) :
        BaseTy(wistd::move(rhs))
    {
    }

    BasicActivity& operator=(BasicActivity&& rhs)
    {
        BaseTy::operator=(wistd::move(rhs));
        return *this;
    }

    /*
    Returns a handle to the TraceLogging provider associated with this activity.
    */
    TraceLoggingHProvider Provider() const
    {
        return TraceLoggingType::Provider();
    }

    /*
    Sets the related (parent) activity.
    May only be called once. If used, must be called before starting the activity.
    */
    template<typename ActivityTy>
    void SetRelatedActivity(_In_ const ActivityTy& relatedActivity)
    {
        SetRelatedId(*relatedActivity.Id());
    }

    /*
    Sets the related (parent) activity.
    May only be called once. If used, must be called before starting the activity.
    */
    void SetRelatedActivityId(_In_ const GUID& relatedActivityId)
    {
        SetRelatedId(relatedActivityId);
    }

    /*
    Sets the related (parent) activity.
    May only be called once. If used, must be called before starting the activity.
    */
    void SetRelatedActivityId(_In_ const GUID* relatedActivityId)
    {
        _TLG_ASSERT(relatedActivityId != NULL, "TraceLoggingActivity SetRelatedActivity called with NULL id.");
        SetRelatedId(*relatedActivityId);
    }
};

template<
    typename TraceLoggingType,
    UINT64 keyword = 0,
    UINT8 level = WINEVENT_LEVEL_VERBOSE,
    typename TlgReflectorTag = _TlgReflectorTag_Param0IsProviderType> // helps TlgReflector understand that this is a wrapper type
class BasicThreadActivity
    : public _TlgActivityBase<BasicThreadActivity<TraceLoggingType, keyword, level, TlgReflectorTag>, keyword, level>
{
    typedef
        _TlgActivityBase<BasicThreadActivity<TraceLoggingType, keyword, level, TlgReflectorTag>, keyword, level>
        BaseTy;
    friend class BaseTy;

    void OnStarted()
    {
        PushThreadActivityId();
    }

    void OnStopped()
    {
        PopThreadActivityId();
    }

public:

    BasicThreadActivity()
    {
    }

    BasicThreadActivity(BasicThreadActivity&& rhs)
        : BaseTy(wistd::move(rhs))
    {
    }

    BasicThreadActivity& operator=(BasicThreadActivity&& rhs)
    {
        BaseTy::operator=(wistd::move(rhs));
        return *this;
    }

    /*
    Returns a handle to the TraceLogging provider associated with this activity.
    */
    TraceLoggingHProvider Provider() const
    {
        return TraceLoggingType::Provider();
    }
};

#define __WI_TraceLoggingWriteTagged(activity, name, ...) \
    __pragma(warning(push)) __pragma(warning(disable:4127)) \
    do { \
        _TlgActivityDecl(activity) \
        TraceLoggingWriteActivity( \
            TraceLoggingType::Provider(), \
            (name), \
            _TlgActivityRef(activity).Id(), \
            NULL, \
            __VA_ARGS__); \
    } while(0) \
    __pragma(warning(pop)) \


// This is the ultimate base class implementation for all activities.  Activity classes are defined with
// DEFINE_TRACELOGGING_ACTIVITY, DEFINE_CALLCONTEXT_ACTIVITY, DEFINE_TELEMETRY_ACTIVITY and others


template <typename ActivityTraceLoggingType, 
          ActivityOptions options = ActivityOptions::None, UINT64 keyword = 0, UINT8 level = WINEVENT_LEVEL_VERBOSE,
          typename TlgReflectorTag = _TlgReflectorTag_Param0IsProviderType>
class ActivityBase : public details::IFailureCallback
{
public:
    typedef ActivityTraceLoggingType TraceLoggingType;

    static UINT64 const Keyword = keyword;
    static UINT8 const Level = level;

    ActivityBase(PCSTR contextName, bool shouldWatchErrors = false) WI_NOEXCEPT :
        m_activityData(contextName),
        m_pActivityData(&m_activityData),
        m_callbackHolder(this, m_activityData.GetCallContext(), shouldWatchErrors)
    {
    }

    ActivityBase(ActivityBase &&other, bool shouldWatchErrors) WI_NOEXCEPT :
        m_activityData(wistd::move(other.m_activityData)),
        m_sharedActivityData(wistd::move(other.m_sharedActivityData)),
        m_callbackHolder(this, nullptr, shouldWatchErrors)
    {
        m_pActivityData = m_sharedActivityData ? m_sharedActivityData.get() : &m_activityData;
        m_callbackHolder.SetCallContext(m_pActivityData->GetCallContext());
        other.m_pActivityData = &other.m_activityData;
        if (other.m_callbackHolder.IsWatching())
        {
            other.m_callbackHolder.StopWatching();
        }
    }

    ActivityBase(ActivityBase &&other) WI_NOEXCEPT :
        ActivityBase(wistd::move(other), other.m_callbackHolder.IsWatching())
    {
    }

    ActivityBase(ActivityBase const &other) WI_NOEXCEPT :
        m_activityData(),
        m_pActivityData(&m_activityData),
        m_callbackHolder(this, false)                           // false = do not automatically watch for failures
    {
        operator=(other);
    }

    ActivityBase& operator=(ActivityBase &&other) WI_NOEXCEPT
    {
        m_activityData = wistd::move(other.m_activityData);
        m_sharedActivityData = wistd::move(other.m_sharedActivityData);
        m_pActivityData = m_sharedActivityData ? m_sharedActivityData.get() : &m_activityData;
        m_callbackHolder.SetCallContext(m_pActivityData->GetCallContext());
        m_callbackHolder.SetWatching(other.m_callbackHolder.IsWatching());
        other.m_pActivityData = &other.m_activityData;
        if (other.m_callbackHolder.IsWatching())
        {
            other.m_callbackHolder.StopWatching();
        }
        return *this;
    }

    ActivityBase& operator=(ActivityBase const &other) WI_NOEXCEPT
    {
        if (m_callbackHolder.IsWatching())
        {
            m_callbackHolder.StopWatching();
        }

        if (other.m_sharedActivityData)
        {
            m_pActivityData = other.m_pActivityData;
            m_sharedActivityData = other.m_sharedActivityData;
        }
        else if (m_sharedActivityData.create(wistd::move(other.m_activityData)))
        {
            // Locking should not be required as the first copy should always take place on the owning
            // thread...
            m_pActivityData = m_sharedActivityData.get();
            other.m_sharedActivityData = m_sharedActivityData;
            other.m_pActivityData = m_pActivityData;
            other.m_callbackHolder.SetCallContext(m_pActivityData->GetCallContext());
        }
        m_callbackHolder.SetCallContext(m_pActivityData->GetCallContext());
        return *this;
    }

    // These calls all result in setting a message to associate with any failures that might occur while
    // running the activity.  For example, you could associate a filename with a call context activity
    // so that the file name is only reported if the activity fails with the failure.

    void SetMessage(_In_ _Printf_format_string_ PCSTR formatString, ...)
    {
        va_list argList;
        va_start(argList, formatString);
        auto lock = LockExclusive();
        GetCallContext()->SetMessage(formatString, argList);
    }

    void SetMessage(_In_opt_ PCWSTR message)
    {
        auto lock = LockExclusive();
        GetCallContext()->SetMessage(message);
    }

    void SetMessageCopy(_In_opt_ PCWSTR message)
    {
        auto lock = LockExclusive();
        GetCallContext()->SetMessageCopy(message);
    }

    // This call stops watching for errors on the thread that the activity was originally
    // created on.  Use it when moving the activity into a thread-agnostic class or moving
    // an activity across threads.

    void IgnoreCurrentThread() WI_NOEXCEPT
    {
        if (m_callbackHolder.IsWatching())
        {
            m_callbackHolder.StopWatching();
        }
    }

    // Call this API to retrieve an RAII object to watch events on the current thread.  The returned
    // object should only be used on the stack.

    _Check_return_ ActivityThreadWatcher ContinueOnCurrentThread() WI_NOEXCEPT
    {
        if (IsRunning())
        {
            __WI_TraceLoggingWriteTagged(*this, "ActivityContinue", TraceLoggingKeyword(Keyword), __ACTIVITY_CONTINUE_PARAMS());

            return ActivityThreadWatcher(this, *m_pActivityData->GetCallContext());
        }
        return ActivityThreadWatcher();
    }

    // This is the 'default' Stop routine that accepts an HRESULT and completes the activity...

    void Stop(HRESULT hr = S_OK) WI_NOEXCEPT
    {
        bool stopActivity;
        HRESULT hrLocal;
        {
            auto lock = LockExclusive();
            stopActivity = m_pActivityData->SetStopResult(hr, &hrLocal);
        }
        if (stopActivity)
        {
            ReportStopActivity(hrLocal);
        }
        else
        {
            __WI_TraceLoggingWriteTagged(*this, "ActivityIntermediateStop", TraceLoggingKeyword(Keyword), __ACTIVITY_STOP_PARAMS(hr));
        }
        IgnoreCurrentThread();
    }

    // IFailureCallback

    bool NotifyFailure(FailureInfo const &failure) WI_NOEXCEPT override
    {
        // We always report errors to the ETW stream, but we hold-back the telemetry keyword if we've already reported this error to this
        // particular telemetry provider.

        __TRACELOGGING_TEST_HOOK_ACTIVITY_ERROR(failure);

#pragma warning(push)
#pragma warning(disable: 6319)
        if (false, WI_IS_FLAG_SET(options, ActivityOptions::TelemetryOnFailure) && !WasAlreadyReportedToTelemetry(failure.failureId))
        {
            __WI_TraceLoggingWriteTagged(*this, "ActivityError", TraceLoggingKeyword(Keyword | MICROSOFT_KEYWORD_TELEMETRY), __ACTIVITY_ERROR_TELEMETRY_FAILURE_PARAMS(failure));
        }
        else
        {
            __WI_TraceLoggingWriteTagged(*this, "ActivityError", TraceLoggingKeyword(Keyword), __ACTIVITY_ERROR_TRACELOGGING_FAILURE_PARAMS(failure));
        }
#pragma warning(pop)

        auto lock = LockExclusive();
        m_pActivityData->NotifyFailure(failure);
        return true;
    }

    // This is the base TraceLoggingActivity<> contract...  we implement it so that this class
    // can be used by all of the activity macros and we re-route the request as needed.
    //
    // The contract required by the TraceLogging Activity macros is:
    // - activity.Keyword // compile-time constant
    // - activity.Level   // compile-time constant
    // - activity.Provider()
    // - activity.Id()
    // - activity.zInternalRelatedId()
    // - activity.zInternalStart()
    // - activity.zInternalStop()
    // In addition, for TlgReflector to work correctly, it must be possible for
    // TlgReflector to statically map from typeof(activity) to hProvider.

    GUID const* zInternalRelatedId() const WI_NOEXCEPT
    { return m_pActivityData->zInternalRelatedId(); }

    void zInternalStart() WI_NOEXCEPT
    { auto lock = LockExclusive(); m_pActivityData->zInternalStart(); }

    void zInternalStop() WI_NOEXCEPT
    { auto lock = LockExclusive(); m_pActivityData->zInternalStop(); }

    static TraceLoggingHProvider const Provider() WI_NOEXCEPT
    { return ActivityTraceLoggingType::Provider(); }

    GUID const* Id() const WI_NOEXCEPT 
    { return m_pActivityData->Id(); }

    GUID const* providerGuid() const WI_NOEXCEPT
    { return m_pActivityData->providerGuid(); }

    template<class OtherTy>
    void SetRelatedActivity(OtherTy const &relatedActivity) WI_NOEXCEPT
    { auto lock = LockExclusive(); m_pActivityData->SetRelatedActivityId(relatedActivity.Id()); }

    void SetRelatedActivityId(_In_ const GUID& relatedActivityId) WI_NOEXCEPT
    { auto lock = LockExclusive(); m_pActivityData->SetRelatedActivityId(&relatedActivityId); }

    void SetRelatedActivityId(_In_ const GUID* relatedActivityId) WI_NOEXCEPT
    { auto lock = LockExclusive(); m_pActivityData->SetRelatedActivityId(relatedActivityId); }
protected:
    virtual void StopActivity() WI_NOEXCEPT = 0;
    virtual bool WasAlreadyReportedToTelemetry(long failureId) WI_NOEXCEPT = 0;

    void EnsureWatchingCurrentThread()
    {
        if (!m_callbackHolder.IsWatching())
        {
            m_callbackHolder.StartWatching();
        }
    }

    void SetStopResult(HRESULT hr, _Out_opt_ HRESULT *phr = nullptr) WI_NOEXCEPT
    { auto lock = LockExclusive(); m_pActivityData->SetStopResult(hr, phr); }

    void IncrementExpectedStopCount() WI_NOEXCEPT
    { auto lock = LockExclusive(); m_pActivityData->IncrementExpectedStopCount(); }

    // Locking should not be required on these accessors as we only use this at reporting (which will only happen from
    // the final stop)

    FailureInfo const * GetFailureInfo() WI_NOEXCEPT
    { return m_pActivityData->GetFailureInfo(); }

    inline HRESULT GetResult() const WI_NOEXCEPT
    { return m_pActivityData->GetResult(); }

    details::StoredCallContextInfo *GetCallContext() const WI_NOEXCEPT
    {
        return m_pActivityData->GetCallContext();
    }

    // Think of this routine as the destructor -- since we need to call virtual derived methods, we can't use it as
    // a destructor without a pure virtual method call, so we have the derived class call it in its destructor...

    void Destroy() WI_NOEXCEPT
    {
        bool fStop = true;
        if (m_sharedActivityData)
        {
            // The lock unifies the 'unique()' check and the 'reset()' of any non-unique activity so that we
            // can positively identify the final release of the internal data

            auto lock = LockExclusive();
            if (!m_sharedActivityData.unique())
            {
                fStop = false;
                m_sharedActivityData.reset();
            }
        }

        if (fStop && m_pActivityData->NeedsStopped())
        {
            ReportStopActivity(m_pActivityData->SetUnhandledException());
        }
    }

    inline bool IsRunning() const WI_NOEXCEPT
    {
        return m_pActivityData->NeedsStopped();
    }

private:
    void ReportStopActivity(HRESULT hr) WI_NOEXCEPT
    {
        if (FAILED(hr) && WI_ARE_ALL_FLAGS_CLEAR(Keyword, (MICROSOFT_KEYWORD_TELEMETRY | MICROSOFT_KEYWORD_MEASURES | MICROSOFT_KEYWORD_CRITICAL_DATA)) && WI_IS_FLAG_SET(options, ActivityOptions::TelemetryOnFailure))
        {
            wil::FailureInfo const* pFailure = GetFailureInfo();
            if (pFailure != nullptr)
            {
                __TRACELOGGING_TEST_HOOK_CALLCONTEXT_ERROR(pFailure, pFailure->hr);
                auto & failure = *pFailure;
                __WI_TraceLoggingWriteTagged(*this, "ActivityFailure", TraceLoggingKeyword(Keyword | MICROSOFT_KEYWORD_TELEMETRY), __ACTIVITY_FAILURE_TELEMETRY_FAILURE_PARAMS(failure));
            }
            else
            {
                __TRACELOGGING_TEST_HOOK_CALLCONTEXT_ERROR(nullptr, hr);
                __WI_TraceLoggingWriteTagged(*this, "ActivityFailure", TraceLoggingKeyword(Keyword | MICROSOFT_KEYWORD_TELEMETRY),
                                        __ACTIVITY_FAILURE_TELEMETRY_PARAMS(hr, m_pActivityData->GetCallContext()->contextName, m_pActivityData->GetCallContext()->contextMessage));
            }
        }

        StopActivity();
    }

    rwlock_release_exclusive_scope_exit LockExclusive() WI_NOEXCEPT
    {
        // We only need to lock when we're sharing....
        return (m_sharedActivityData ? m_sharedActivityData->LockExclusive() : rwlock_release_exclusive_scope_exit());
    }

    template <typename ActivityTraceLoggingType, 
              typename TlgReflectorTag = _TlgReflectorTag_Param0IsProviderType>
    class ActivityData :
        public _TlgActivityBase<ActivityData<ActivityTraceLoggingType, TlgReflectorTag>, keyword, level>
    {
        typedef _TlgActivityBase<ActivityData<ActivityTraceLoggingType, TlgReflectorTag>, keyword, level> BaseTy;
        friend class BaseTy;
        void OnStarted() {}
        void OnStopped() {}

        // SFINAE dispatching on presence of ActivityTraceLoggingType::CreateActivityId(_Out_ GUID& childActivityId, _In_opt_ const GUID* relatedActivityId)
        template<typename ProviderType>
        auto CreateActivityIdByProviderType(int, _Out_ GUID& childActivityId) ->
            decltype(ProviderType::CreateActivityId(childActivityId, zInternalRelatedId()), (void)0)
        {
            ProviderType::CreateActivityId(childActivityId, zInternalRelatedId());
        }

        template<typename ProviderType>
        auto CreateActivityIdByProviderType(long, _Out_ GUID& childActivityId) ->
            void
        {
            EventActivityIdControl(EVENT_ACTIVITY_CTRL_CREATE_ID, &childActivityId);
        }

        void CreateActivityId(_Out_ GUID& childActivityId)
        {
            CreateActivityIdByProviderType<ActivityTraceLoggingType>(0, childActivityId);
        }

    public:
        ActivityData(_In_opt_ PCSTR contextName = nullptr) WI_NOEXCEPT :
            BaseTy(),
            m_callContext(contextName),
            m_result(S_OK),
            m_stopCountExpected(1)
        {
        }

        ActivityData(ActivityData &&other) WI_NOEXCEPT :
            BaseTy(wistd::move(other)),
            m_callContext(wistd::move(other.m_callContext)),
            m_result(other.m_result),
            m_failure(wistd::move(other.m_failure)),
            m_stopCountExpected(other.m_stopCountExpected)
        {
        }

        ActivityData & operator=(ActivityData &&other) WI_NOEXCEPT
        {
            BaseTy::operator=(wistd::move(other));
            m_callContext = wistd::move(other.m_callContext);
            m_result = other.m_result;
            m_failure = wistd::move(other.m_failure);
            m_stopCountExpected = other.m_stopCountExpected;
            return *this;
        }

        ActivityData(ActivityData const &other) = delete;
        ActivityData & operator=(ActivityData const &other) = delete;

        // returns true if the event was reported to telemetry
        void NotifyFailure(FailureInfo const &failure) WI_NOEXCEPT
        {
            if ((failure.hr != m_failure.GetFailureInfo().hr) &&      // don't replace with the same error (likely propagation up the stack)
               ((failure.hr != m_result) || SUCCEEDED(m_result)))     // don't replace if we've already got the current explicitly supplied failure code
            {
                m_failure.SetFailureInfo(failure);
            }
        }

        rwlock_release_exclusive_scope_exit LockExclusive() WI_NOEXCEPT
        {
            return m_lock.lock_exclusive();
        }

        static TraceLoggingHProvider const Provider()
        {
            return ActivityTraceLoggingType::Provider();
        }

        bool NeedsStopped() const WI_NOEXCEPT
        {
            return IsStarted();
        }

        void SetRelatedActivityId(const GUID* relatedId)
        {
            SetRelatedId(*relatedId);
        }

        bool SetStopResult(HRESULT hr, _Out_opt_ HRESULT *phr) WI_NOEXCEPT
        {
            // We must be expecting at least one Stop -- otherwise the caller is calling Stop() more times
            // than it can (normally once, or +1 for each call to Split())
            __FAIL_FAST_IMMEDIATE_ASSERT__(m_stopCountExpected >= 1);
            if (SUCCEEDED(m_result))
            {
                m_result = hr;
            }
            if (phr != nullptr)
            {
                *phr = m_result;
            }
            return ((--m_stopCountExpected) == 0);
        }

        HRESULT SetUnhandledException() WI_NOEXCEPT
        {
            HRESULT hr = m_failure.GetFailureInfo().hr;
            SetStopResult(FAILED(hr) ? hr : HRESULT_FROM_WIN32(ERROR_UNHANDLED_EXCEPTION), &hr);
            return hr;
        }

        void IncrementExpectedStopCount() WI_NOEXCEPT
        {
            m_stopCountExpected++;
        }
        
        FailureInfo const *GetFailureInfo() const WI_NOEXCEPT
        {
            return (FAILED(m_result) && (m_result == m_failure.GetFailureInfo().hr)) ? &m_failure.GetFailureInfo() : nullptr;
        }

        inline HRESULT GetResult() const WI_NOEXCEPT
        {
            return m_result;
        }

        details::StoredCallContextInfo *GetCallContext() WI_NOEXCEPT
        {
            return &m_callContext;
        }

    private:
        details::StoredCallContextInfo m_callContext;
        int m_stopCountExpected;
        HRESULT m_result;
        StoredFailureInfo m_failure;
        wil::srwlock m_lock;
    };

    mutable details::ThreadFailureCallbackHolder m_callbackHolder;
    mutable ActivityData<ActivityTraceLoggingType, TlgReflectorTag> *m_pActivityData;
    mutable ActivityData<ActivityTraceLoggingType, TlgReflectorTag> m_activityData;
    mutable details::shared_object<ActivityData<ActivityTraceLoggingType, TlgReflectorTag>> m_sharedActivityData;
};

} // namespace wil


// Internal MACRO implementation of Activities.
// Do NOT use these macros directly.

#define __WI_TraceLoggingWriteStart(activity, name, ...) \
    __pragma(warning(push)) __pragma(warning(disable:4127)) \
    do { \
        _TlgActivityDecl(activity) \
        static const UINT64 _TlgActivity_Keyword = _TlgActivityRef(activity).Keyword;\
        static const UINT8 _TlgActivity_Level = _TlgActivityRef(activity).Level;\
        _TLG_CASSERT( \
            _TlgActivity_Keyword == (_TlgActivity_Keyword _TLG_FOREACH(_TlgKeywordVal, __VA_ARGS__)), \
            "Do not use TraceLoggingKeyword in TraceLoggingWriteStart. Keywords for START events are " \
            "specified in the activity type, e.g. TraceLoggingActivity<Provider,Keyword,Level>."); \
        _TLG_CASSERT( \
            _TlgActivity_Level == (_TlgActivity_Level _TLG_FOREACH(_TlgLevelVal, __VA_ARGS__)), \
            "Do not use TraceLoggingLevel in TraceLoggingWriteStart. The Level for START events is " \
            "specified in the activity type, e.g. TraceLoggingActivity<Provider,Keyword,Level>."); \
        _TlgActivityRef(activity).zInternalStart(); \
        TraceLoggingWriteActivity( \
            TraceLoggingType::Provider(), \
            (name), \
            _TlgActivityRef(activity).Id(), \
            _TlgActivityRef(activity).zInternalRelatedId(), \
            TraceLoggingOpcode(1 /* WINEVENT_OPCODE_START */), \
            TraceLoggingKeyword(_TlgActivity_Keyword), \
            TraceLoggingLevel(_TlgActivity_Level), \
            TraceLoggingDescription("~^" _TLG_PASTE(L, _TLG_STRINGIZE(activity)) L"^~"), \
            __VA_ARGS__); \
    } while(0) \
    __pragma(warning(pop)) \

#define __WRITE_ACTIVITY_START(EventId, ...) \
    __TRACELOGGING_TEST_HOOK_ACTIVITY_START(); \
    __WI_TraceLoggingWriteStart(*this, #EventId, __ACTIVITY_START_PARAMS(), __VA_ARGS__); \
    EnsureWatchingCurrentThread()

#define __WI_TraceLoggingWriteStop(activity, name, ...) \
    __pragma(warning(push)) __pragma(warning(disable:4127)) \
    do { \
        _TlgActivityDecl(activity) \
        static const UINT64 _TlgActivity_Keyword = _TlgActivityRef(activity).Keyword;\
        static const UINT8 _TlgActivity_Level = _TlgActivityRef(activity).Level;\
        _TLG_CASSERT( \
            _TlgActivity_Keyword == (_TlgActivity_Keyword _TLG_FOREACH(_TlgKeywordVal, __VA_ARGS__)), \
            "Do not use TraceLoggingKeyword in TraceLoggingWriteStop. Keywords for STOP events are " \
            "specified in the activity type, e.g. TraceLoggingActivity<Provider,Keyword,Level>."); \
        _TLG_CASSERT( \
            _TlgActivity_Level == (_TlgActivity_Level _TLG_FOREACH(_TlgLevelVal, __VA_ARGS__)), \
            "Do not use TraceLoggingLevel in TraceLoggingWriteStop. The Level for STOP events is " \
            "specified in the activity type, e.g. TraceLoggingActivity<Provider,Keyword,Level>."); \
        _TlgActivityRef(activity).zInternalStop(); \
        TraceLoggingWriteActivity( \
            TraceLoggingType::Provider(), \
            (name), \
            _TlgActivityRef(activity).Id(), \
            NULL, \
            TraceLoggingOpcode(2 /* WINEVENT_OPCODE_STOP */),\
            TraceLoggingKeyword(_TlgActivity_Keyword),\
            TraceLoggingLevel(_TlgActivity_Level),\
            TraceLoggingDescription("~^" _TLG_PASTE(L, _TLG_STRINGIZE(activity)) L"^~"),\
            __VA_ARGS__); \
    } while(0) \
    __pragma(warning(pop)) \

#define __WRITE_ACTIVITY_STOP(EventId, ...) \
    wil::FailureInfo const* pFailure = GetFailureInfo(); \
    if (pFailure != nullptr) \
        { \
        __TRACELOGGING_TEST_HOOK_ACTIVITY_STOP(pFailure, pFailure->hr); \
        auto &failure = *pFailure; \
        if (false, WI_IS_ANY_FLAG_SET(Keyword, (MICROSOFT_KEYWORD_TELEMETRY | MICROSOFT_KEYWORD_MEASURES | MICROSOFT_KEYWORD_CRITICAL_DATA))) \
        { \
            __WI_TraceLoggingWriteStop(*this, #EventId, __ACTIVITY_STOP_TELEMETRY_FAILURE_PARAMS(failure), __VA_ARGS__); \
        } \
        else \
        { \
            __WI_TraceLoggingWriteStop(*this, #EventId, __ACTIVITY_STOP_TRACELOGGING_FAILURE_PARAMS(failure), __VA_ARGS__); \
        } \
    } \
    else \
    { \
        __TRACELOGGING_TEST_HOOK_ACTIVITY_STOP(nullptr, GetResult()); \
        __WI_TraceLoggingWriteStop(*this, #EventId, __ACTIVITY_STOP_PARAMS(GetResult()), __VA_ARGS__); \
    } \
    IgnoreCurrentThread();

// optional params are:  KeyWord, Level, Options
#define __BEGIN_TRACELOGGING_ACTIVITY_CLASS(ActivityClassName, ...) \
    class ActivityClassName final : public wil::ActivityBase<TraceLoggingType, __VA_ARGS__> \
    { \
    protected: \
        virtual void StopActivity() WI_NOEXCEPT override \
            { __WRITE_ACTIVITY_STOP(ActivityClassName); } \
        virtual bool WasAlreadyReportedToTelemetry(long failureId) WI_NOEXCEPT override \
            { return TraceLoggingType::WasAlreadyReportedToTelemetry(failureId); } \
    public: \
        static bool IsEnabled() WI_NOEXCEPT \
            { return TraceLoggingType::IsEnabled(); } \
        ~ActivityClassName() WI_NOEXCEPT { ActivityBase::Destroy(); } \
        ActivityClassName(ActivityClassName const &other) WI_NOEXCEPT : ActivityBase(other) {} \
        ActivityClassName(ActivityClassName &&other) WI_NOEXCEPT : ActivityBase(wistd::move(other)) {} \
        ActivityClassName(ActivityClassName &&other, bool shouldWatchErrors) WI_NOEXCEPT : ActivityBase(wistd::move(other), shouldWatchErrors) {} \
        ActivityClassName& operator=(ActivityClassName const &other) WI_NOEXCEPT \
            { ActivityBase::operator=(other); return *this; } \
        ActivityClassName& operator=(ActivityClassName &&other) WI_NOEXCEPT \
            { auto localActivity(wistd::move(*this)); ActivityBase::operator=(wistd::move(other)); return *this; } \
        explicit operator bool() const WI_NOEXCEPT \
            { return IsRunning(); } \
        void StopWithResult(HRESULT hr) \
            { ActivityBase::Stop(hr); } \
        template<typename... TArgs> \
        void StopWithResult(HRESULT hr, TArgs&&... args) \
            { SetStopResult(hr); Stop(wistd::forward<TArgs>(args)...); } \
        void Stop(HRESULT hr = S_OK) WI_NOEXCEPT \
            { ActivityBase::Stop(hr); } \
        void StartActivity() WI_NOEXCEPT \
            { __WRITE_ACTIVITY_START(ActivityClassName); } \
        void StartRelatedActivity() WI_NOEXCEPT \
            { wil::details::SetRelatedActivityId(*this); StartActivity(); } \
        void StartActivityWithCorrelationVector(PCSTR correlationVector) WI_NOEXCEPT \
            { __WRITE_ACTIVITY_START(ActivityClassName, TraceLoggingString(correlationVector, "__TlgCV__")); } \
        _Check_return_ ActivityClassName Split() WI_NOEXCEPT \
            { __FAIL_FAST_IMMEDIATE_ASSERT__(IsRunning()); IncrementExpectedStopCount(); return ActivityClassName(*this); } \
        _Check_return_ ActivityClassName TransferToCurrentThread() WI_NOEXCEPT \
            { return ActivityClassName(wistd::move(*this), IsRunning()); } \
        _Check_return_ ActivityClassName TransferToMember() WI_NOEXCEPT \
            { return ActivityClassName(wistd::move(*this), false); }

#define __IMPLEMENT_ACTIVITY_CLASS(ActivityClassName) \
    private: \
        template<typename... TArgs> \
        ActivityClassName(wil::details::tag_start, TArgs&&... args) WI_NOEXCEPT : ActivityBase(#ActivityClassName) \
            { StartActivity(wistd::forward<TArgs>(args)...); \
              __TRACELOGGING_DEFINE_PROVIDER_STORAGE_LINK("this", ActivityClassName); } \
        template<typename... TArgs> \
        ActivityClassName(wil::details::tag_start_cv, _In_opt_ PCSTR correlationVector, TArgs&&... args) WI_NOEXCEPT : ActivityBase(#ActivityClassName) \
            { StartActivityWithCorrelationVector(correlationVector, wistd::forward<TArgs>(args)...); \
              __TRACELOGGING_DEFINE_PROVIDER_STORAGE_LINK("this", ActivityClassName); } \
    public: \
        ActivityClassName() WI_NOEXCEPT : ActivityBase(#ActivityClassName, false) {} \
        template<typename... TArgs> \
        _Check_return_ static ActivityClassName Start(TArgs&&... args) \
            { return ActivityClassName(wil::details::tag_start(), wistd::forward<TArgs>(args)...); } \
        template<typename... TArgs> \
        _Check_return_ static ActivityClassName StartWithCorrelationVector(_In_ PCSTR correlationVector, TArgs&&... args) \
            { return ActivityClassName(wil::details::tag_start_cv(), correlationVector, wistd::forward<TArgs>(args)...); } 

#define __IMPLEMENT_CALLCONTEXT_CLASS(ActivityClassName) \
    protected: \
        ActivityClassName(_In_ void **, PCSTR contextName, _In_opt_ _Printf_format_string_ PCSTR formatString, _In_opt_ va_list argList) : \
            ActivityBase(contextName) \
            { GetCallContext()->SetMessage(formatString, argList); StartActivity(); } \
        ActivityClassName(_In_ void **, PCSTR contextName) : \
            ActivityBase(contextName) \
            { StartActivity(); } \
    public: \
        ActivityClassName(PCSTR contextName) : ActivityBase(contextName, false) {} \
        ActivityClassName(PCSTR contextName, _Printf_format_string_ PCSTR formatString, ...) WI_NOEXCEPT : ActivityClassName(contextName) \
            { va_list argList; va_start(argList, formatString); GetCallContext()->SetMessage(formatString, argList); } \
        _Check_return_ static ActivityClassName Start(PCSTR contextName) WI_NOEXCEPT \
            { return ActivityClassName(static_cast<void **>(__nullptr), contextName); } \
        _Check_return_ static ActivityClassName Start(PCSTR contextName, _Printf_format_string_ PCSTR formatString, ...) WI_NOEXCEPT \
            { va_list argList; va_start(argList, formatString); return ActivityClassName(static_cast<void **>(__nullptr), contextName, formatString, argList); }

#define __END_TRACELOGGING_ACTIVITY_CLASS() \
    };

#define DEFINE_TAGGED_TRACELOGGING_EVENT(EventId, ...) \
    void EventId() { __WI_TraceLoggingWriteTagged(*this, #EventId, __VA_ARGS__); }

#define DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(EventId, VarType1, varName1, ...) \
    template<typename T1> void EventId(T1 &&varName1) \
    { __WI_TraceLoggingWriteTagged(*this, #EventId,  TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), __VA_ARGS__); }

#define DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(EventId, VarType1, varName1, VarType2, varName2, ...) \
    template<typename T1, typename T2> void EventId(T1 &&varName1, T2 &&varName2) \
    { __WI_TraceLoggingWriteTagged(*this, #EventId,  TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), \
                                                TraceLoggingValue(static_cast<VarType2>(wistd::forward<T2>(varName2)), _TLG_STRINGIZE(varName2)), __VA_ARGS__); }

#define DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM3(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, ...) \
    template<typename T1, typename T2, typename T3> void EventId(T1 &&varName1, T2 &&varName2, T3 &&varName3) \
    { __WI_TraceLoggingWriteTagged(*this, #EventId,  TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), \
                                                TraceLoggingValue(static_cast<VarType2>(wistd::forward<T2>(varName2)), _TLG_STRINGIZE(varName2)), \
                                                TraceLoggingValue(static_cast<VarType3>(wistd::forward<T3>(varName3)), _TLG_STRINGIZE(varName3)), __VA_ARGS__); }

#define DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM4(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, ...) \
    template<typename T1, typename T2, typename T3, typename T4> void EventId(T1 &&varName1, T2 &&varName2, T3 &&varName3, T4 &&varName4) \
    { __WI_TraceLoggingWriteTagged(*this, #EventId,  TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), \
                                                TraceLoggingValue(static_cast<VarType2>(wistd::forward<T2>(varName2)), _TLG_STRINGIZE(varName2)), \
                                                TraceLoggingValue(static_cast<VarType3>(wistd::forward<T3>(varName3)), _TLG_STRINGIZE(varName3)), \
                                                TraceLoggingValue(static_cast<VarType4>(wistd::forward<T4>(varName4)), _TLG_STRINGIZE(varName4)), __VA_ARGS__); }

#define DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM5(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, ...) \
    template<typename T1, typename T2, typename T3, typename T4, typename T5> void EventId(T1 &&varName1, T2 &&varName2, T3 &&varName3, T4 &&varName4, T5 &&varName5) \
    { __WI_TraceLoggingWriteTagged(*this, #EventId,  TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), \
                                                TraceLoggingValue(static_cast<VarType2>(wistd::forward<T2>(varName2)), _TLG_STRINGIZE(varName2)), \
                                                TraceLoggingValue(static_cast<VarType3>(wistd::forward<T3>(varName3)), _TLG_STRINGIZE(varName3)), \
                                                TraceLoggingValue(static_cast<VarType4>(wistd::forward<T4>(varName4)), _TLG_STRINGIZE(varName4)), \
                                                TraceLoggingValue(static_cast<VarType5>(wistd::forward<T5>(varName5)), _TLG_STRINGIZE(varName5)), __VA_ARGS__); }

#define DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM6(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, ...) \
    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> void EventId(T1 &&varName1, T2 &&varName2, T3 &&varName3, T4 &&varName4, T5 &&varName5, T6 &&varName6) \
    { __WI_TraceLoggingWriteTagged(*this, #EventId,  TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), \
                                                TraceLoggingValue(static_cast<VarType2>(wistd::forward<T2>(varName2)), _TLG_STRINGIZE(varName2)), \
                                                TraceLoggingValue(static_cast<VarType3>(wistd::forward<T3>(varName3)), _TLG_STRINGIZE(varName3)), \
                                                TraceLoggingValue(static_cast<VarType4>(wistd::forward<T4>(varName4)), _TLG_STRINGIZE(varName4)), \
                                                TraceLoggingValue(static_cast<VarType5>(wistd::forward<T5>(varName5)), _TLG_STRINGIZE(varName5)), \
                                                TraceLoggingValue(static_cast<VarType6>(wistd::forward<T6>(varName6)), _TLG_STRINGIZE(varName6)), __VA_ARGS__); }

#define DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM7(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, ...) \
    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> void EventId(T1 &&varName1, T2 &&varName2, T3 &&varName3, T4 &&varName4, T5 &&varName5, T6 &&varName6, T7 &&varName7) \
    { __WI_TraceLoggingWriteTagged(*this, #EventId,  TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), \
                                                TraceLoggingValue(static_cast<VarType2>(wistd::forward<T2>(varName2)), _TLG_STRINGIZE(varName2)), \
                                                TraceLoggingValue(static_cast<VarType3>(wistd::forward<T3>(varName3)), _TLG_STRINGIZE(varName3)), \
                                                TraceLoggingValue(static_cast<VarType4>(wistd::forward<T4>(varName4)), _TLG_STRINGIZE(varName4)), \
                                                TraceLoggingValue(static_cast<VarType5>(wistd::forward<T5>(varName5)), _TLG_STRINGIZE(varName5)), \
                                                TraceLoggingValue(static_cast<VarType6>(wistd::forward<T6>(varName6)), _TLG_STRINGIZE(varName6)), \
                                                TraceLoggingValue(static_cast<VarType7>(wistd::forward<T7>(varName7)), _TLG_STRINGIZE(varName7)), __VA_ARGS__); }

#define DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM8(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, VarType8, varName8, ...) \
    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> void EventId(T1 &&varName1, T2 &&varName2, T3 &&varName3, T4 &&varName4, T5 &&varName5, T6 &&varName6, T7 &&varName7, T8 &&varName8) \
    { __WI_TraceLoggingWriteTagged(*this, #EventId,  TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), \
                                                TraceLoggingValue(static_cast<VarType2>(wistd::forward<T2>(varName2)), _TLG_STRINGIZE(varName2)), \
                                                TraceLoggingValue(static_cast<VarType3>(wistd::forward<T3>(varName3)), _TLG_STRINGIZE(varName3)), \
                                                TraceLoggingValue(static_cast<VarType4>(wistd::forward<T4>(varName4)), _TLG_STRINGIZE(varName4)), \
                                                TraceLoggingValue(static_cast<VarType5>(wistd::forward<T5>(varName5)), _TLG_STRINGIZE(varName5)), \
                                                TraceLoggingValue(static_cast<VarType6>(wistd::forward<T6>(varName6)), _TLG_STRINGIZE(varName6)), \
                                                TraceLoggingValue(static_cast<VarType7>(wistd::forward<T7>(varName7)), _TLG_STRINGIZE(varName7)), \
                                                TraceLoggingValue(static_cast<VarType8>(wistd::forward<T8>(varName8)), _TLG_STRINGIZE(varName8)), __VA_ARGS__); }

#define DEFINE_TAGGED_TRACELOGGING_EVENT_UINT32(EventId, varName, ...)  DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(EventId, UINT32, varName, __VA_ARGS__)  
#define DEFINE_TAGGED_TRACELOGGING_EVENT_BOOL(EventId, varName, ...)    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(EventId, bool, varName, __VA_ARGS__)  
#define DEFINE_TAGGED_TRACELOGGING_EVENT_STRING(EventId, varName, ...)  DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(EventId, PCWSTR, varName, __VA_ARGS__)  


// Internal MACRO implementation of TraceLogging classes.
// Do NOT use these macros directly.

#define __IMPLEMENT_TRACELOGGING_CLASS_BASE(TraceLoggingClassName, TraceLoggingProviderOwnerClassName) \
    public: \
        typedef TraceLoggingProviderOwnerClassName TraceLoggingType; \
        static bool IsEnabled() WI_NOEXCEPT \
            { return Instance()->IsEnabled_(); } \
        static TraceLoggingHProvider const Provider() WI_NOEXCEPT \
            { return static_cast<TraceLoggingProvider *>(Instance())->Provider_(); } \
        static void SetTelemetryEnabled(bool) WI_NOEXCEPT {} \
        static void SetErrorReportingType(wil::ErrorReportingType type) WI_NOEXCEPT \
            { return Instance()->SetErrorReportingType_(type); } \
        static void __stdcall FallbackTelemetryCallback(bool alreadyReported, wil::FailureInfo const &failure) WI_NOEXCEPT \
            { return Instance()->OnErrorReported(alreadyReported, failure); } \
        _Check_return_ static wil::ActivityThreadWatcher WatchCurrentThread(PCSTR contextName) WI_NOEXCEPT \
            { return wil::ActivityThreadWatcher(Instance(), contextName); } \
        _Check_return_ static wil::ActivityThreadWatcher WatchCurrentThread(PCSTR contextName, _Printf_format_string_ PCSTR formatString, ...) WI_NOEXCEPT \
            { va_list argList; va_start(argList, formatString); return wil::ActivityThreadWatcher(Instance(), contextName, formatString, argList); } \
        __BEGIN_TRACELOGGING_ACTIVITY_CLASS(CallContext, wil::ActivityOptions::TelemetryOnFailure) \
            __IMPLEMENT_CALLCONTEXT_CLASS(CallContext); \
        __END_TRACELOGGING_ACTIVITY_CLASS(); \
        static CallContext Start(PCSTR contextName) WI_NOEXCEPT \
            { return CallContext(contextName, __nullptr, __nullptr); } \
        static CallContext Start(PCSTR contextName, _Printf_format_string_ PCSTR formatString, ...) WI_NOEXCEPT \
            { va_list argList; va_start(argList, formatString); return CallContext(contextName, formatString, argList); } \
        static void TraceLoggingInfo(_Printf_format_string_ PCSTR formatString, ...) WI_NOEXCEPT \
            { va_list argList; va_start(argList, formatString); return Instance()->ReportTraceLoggingMessage(false, formatString, argList); } \
        static void TraceLoggingError(_Printf_format_string_ PCSTR formatString, ...) WI_NOEXCEPT \
            { va_list argList; va_start(argList, formatString); return Instance()->ReportTraceLoggingMessage(true, formatString, argList); } \
    private: \
        TraceLoggingHProvider const Provider_() const WI_NOEXCEPT = delete; \
        TraceLoggingClassName() WI_NOEXCEPT {}; \
    protected: \
        static TraceLoggingClassName* Instance() WI_NOEXCEPT \
            { static wil::details::static_lazy<TraceLoggingClassName> wrapper; return wrapper.get(); } \
        friend class wil::details::static_lazy<TraceLoggingClassName>; \

#define __IMPLEMENT_TRACELOGGING_CLASS(TraceLoggingClassName, ProviderName, ProviderId) \
    __IMPLEMENT_TRACELOGGING_CLASS_BASE(TraceLoggingClassName, TraceLoggingClassName) \
    private: \
        struct StaticHandle \
        { \
            TraceLoggingHProvider handle; \
            StaticHandle() WI_NOEXCEPT \
            { \
               TRACELOGGING_DEFINE_PROVIDER_STORAGE(__hInner, ProviderName, ProviderId, TraceLoggingOptionMicrosoftTelemetry()); \
               _TlgDefineProvider_annotation(TraceLoggingClassName, _Tlg##TraceLoggingClassName##Prov, 0, ProviderName); \
               handle = &__hInner; \
            } \
        } m_staticHandle; \
    protected: \
        void Create() WI_NOEXCEPT \
            { Register(m_staticHandle.handle); } \
    public:

#define __IMPLEMENT_TRACELOGGING_CLASS_WITHOUT_TELEMETRY(TraceLoggingClassName, ProviderName, ProviderId) \
    __IMPLEMENT_TRACELOGGING_CLASS_BASE(TraceLoggingClassName, TraceLoggingClassName) \
    private: \
        struct StaticHandle \
        { \
            TraceLoggingHProvider handle; \
            StaticHandle() WI_NOEXCEPT \
            { \
               TRACELOGGING_DEFINE_PROVIDER_STORAGE(__hInner, ProviderName, ProviderId); \
               _TlgDefineProvider_annotation(TraceLoggingClassName, _Tlg##TraceLoggingClassName##Prov, 0, ProviderName); \
               handle = &__hInner; \
            } \
        } m_staticHandle; \
    protected: \
        void Create() WI_NOEXCEPT \
            { Register(m_staticHandle.handle); } \
    public:

#define DEFINE_TRACELOGGING_EVENT(EventId, ...) \
    static void EventId() { TraceLoggingWrite(TraceLoggingType::Provider(), #EventId, __VA_ARGS__); }

#define DEFINE_TRACELOGGING_EVENT_PARAM1(EventId, VarType1, varName1, ...) \
    template<typename T1> static void EventId(T1 &&varName1) \
    { TraceLoggingWrite(TraceLoggingType::Provider(), #EventId,   TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), __VA_ARGS__); }

#define DEFINE_TRACELOGGING_EVENT_PARAM2(EventId, VarType1, varName1, VarType2, varName2, ...) \
    template<typename T1, typename T2> static void EventId(T1 &&varName1, T2 &&varName2) \
    { TraceLoggingWrite(TraceLoggingType::Provider(), #EventId,   TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), \
                                                TraceLoggingValue(static_cast<VarType2>(wistd::forward<T2>(varName2)), _TLG_STRINGIZE(varName2)), __VA_ARGS__); }

#define DEFINE_TRACELOGGING_EVENT_PARAM3(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, ...) \
    template<typename T1, typename T2, typename T3> static void EventId(T1 &&varName1, T2 &&varName2, T3 &&varName3) \
    { TraceLoggingWrite(TraceLoggingType::Provider(), #EventId,   TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), \
                                                TraceLoggingValue(static_cast<VarType2>(wistd::forward<T2>(varName2)), _TLG_STRINGIZE(varName2)), \
                                                TraceLoggingValue(static_cast<VarType3>(wistd::forward<T3>(varName3)), _TLG_STRINGIZE(varName3)), __VA_ARGS__); }

#define DEFINE_TRACELOGGING_EVENT_PARAM4(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, ...) \
    template<typename T1, typename T2, typename T3, typename T4> static void EventId(T1 &&varName1, T2 &&varName2, T3 &&varName3, T4 &&varName4) \
    { TraceLoggingWrite(TraceLoggingType::Provider(), #EventId,   TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), \
                                                TraceLoggingValue(static_cast<VarType2>(wistd::forward<T2>(varName2)), _TLG_STRINGIZE(varName2)), \
                                                TraceLoggingValue(static_cast<VarType3>(wistd::forward<T3>(varName3)), _TLG_STRINGIZE(varName3)), \
                                                TraceLoggingValue(static_cast<VarType4>(wistd::forward<T4>(varName4)), _TLG_STRINGIZE(varName4)), __VA_ARGS__); }

#define DEFINE_TRACELOGGING_EVENT_PARAM5(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, ...) \
    template<typename T1, typename T2, typename T3, typename T4, typename T5> static void EventId(T1 &&varName1, T2 &&varName2, T3 &&varName3, T4 &&varName4, T5 &&varName5) \
    { TraceLoggingWrite(TraceLoggingType::Provider(), #EventId,   TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), \
                                                TraceLoggingValue(static_cast<VarType2>(wistd::forward<T2>(varName2)), _TLG_STRINGIZE(varName2)), \
                                                TraceLoggingValue(static_cast<VarType3>(wistd::forward<T3>(varName3)), _TLG_STRINGIZE(varName3)), \
                                                TraceLoggingValue(static_cast<VarType4>(wistd::forward<T4>(varName4)), _TLG_STRINGIZE(varName4)), \
                                                TraceLoggingValue(static_cast<VarType5>(wistd::forward<T5>(varName5)), _TLG_STRINGIZE(varName5)), __VA_ARGS__); }

#define DEFINE_TRACELOGGING_EVENT_PARAM6(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, ...) \
    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> static void EventId(T1 &&varName1, T2 &&varName2, T3 &&varName3, T4 &&varName4, T5 &&varName5, T6 &&varName6) \
    { TraceLoggingWrite(TraceLoggingType::Provider(), #EventId,   TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), \
                                                TraceLoggingValue(static_cast<VarType2>(wistd::forward<T2>(varName2)), _TLG_STRINGIZE(varName2)), \
                                                TraceLoggingValue(static_cast<VarType3>(wistd::forward<T3>(varName3)), _TLG_STRINGIZE(varName3)), \
                                                TraceLoggingValue(static_cast<VarType4>(wistd::forward<T4>(varName4)), _TLG_STRINGIZE(varName4)), \
                                                TraceLoggingValue(static_cast<VarType5>(wistd::forward<T5>(varName5)), _TLG_STRINGIZE(varName5)), \
                                                TraceLoggingValue(static_cast<VarType6>(wistd::forward<T6>(varName6)), _TLG_STRINGIZE(varName6)), __VA_ARGS__); }

#define DEFINE_TRACELOGGING_EVENT_PARAM7(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, ...) \
    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> static void EventId(T1 &&varName1, T2 &&varName2, T3 &&varName3, T4 &&varName4, T5 &&varName5, T6 &&varName6, T7 &&varName7) \
    { TraceLoggingWrite(TraceLoggingType::Provider(), #EventId,   TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), \
                                                TraceLoggingValue(static_cast<VarType2>(wistd::forward<T2>(varName2)), _TLG_STRINGIZE(varName2)), \
                                                TraceLoggingValue(static_cast<VarType3>(wistd::forward<T3>(varName3)), _TLG_STRINGIZE(varName3)), \
                                                TraceLoggingValue(static_cast<VarType4>(wistd::forward<T4>(varName4)), _TLG_STRINGIZE(varName4)), \
                                                TraceLoggingValue(static_cast<VarType5>(wistd::forward<T5>(varName5)), _TLG_STRINGIZE(varName5)), \
                                                TraceLoggingValue(static_cast<VarType6>(wistd::forward<T6>(varName6)), _TLG_STRINGIZE(varName6)), \
                                                TraceLoggingValue(static_cast<VarType7>(wistd::forward<T7>(varName7)), _TLG_STRINGIZE(varName7)), __VA_ARGS__); }

#define DEFINE_TRACELOGGING_EVENT_PARAM8(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, VarType8, varName8, ...) \
    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> static void EventId(T1 &&varName1, T2 &&varName2, T3 &&varName3, T4 &&varName4, T5 &&varName5, T6 &&varName6, T7 &&varName7, T8 &&varName8) \
    { TraceLoggingWrite(TraceLoggingType::Provider(), #EventId,   TraceLoggingValue(static_cast<VarType1>(wistd::forward<T1>(varName1)), _TLG_STRINGIZE(varName1)), \
                                                TraceLoggingValue(static_cast<VarType2>(wistd::forward<T2>(varName2)), _TLG_STRINGIZE(varName2)), \
                                                TraceLoggingValue(static_cast<VarType3>(wistd::forward<T3>(varName3)), _TLG_STRINGIZE(varName3)), \
                                                TraceLoggingValue(static_cast<VarType4>(wistd::forward<T4>(varName4)), _TLG_STRINGIZE(varName4)), \
                                                TraceLoggingValue(static_cast<VarType5>(wistd::forward<T5>(varName5)), _TLG_STRINGIZE(varName5)), \
                                                TraceLoggingValue(static_cast<VarType6>(wistd::forward<T6>(varName6)), _TLG_STRINGIZE(varName6)), \
                                                TraceLoggingValue(static_cast<VarType7>(wistd::forward<T7>(varName7)), _TLG_STRINGIZE(varName7)), \
                                                TraceLoggingValue(static_cast<VarType8>(wistd::forward<T8>(varName8)), _TLG_STRINGIZE(varName8)), __VA_ARGS__); }

#define DEFINE_TRACELOGGING_EVENT_UINT32(EventId, varName, ...)  DEFINE_TRACELOGGING_EVENT_PARAM1(EventId, UINT32, varName, __VA_ARGS__)  
#define DEFINE_TRACELOGGING_EVENT_BOOL(EventId, varName, ...)    DEFINE_TRACELOGGING_EVENT_PARAM1(EventId, bool, varName, __VA_ARGS__)  
#define DEFINE_TRACELOGGING_EVENT_STRING(EventId, varName, ...)  DEFINE_TRACELOGGING_EVENT_PARAM1(EventId, PCWSTR, varName, __VA_ARGS__)  


// Declaring a pure TraceLogging class
// To declare a tracelogging class, declare your class derived from wil::TraceLoggingProvider, populate the uuid
// attribute of the class with the GUID of your provider, and then include the IMPLEMENT_TRACELOGGING_CLASS 
// macro within your class.
//
// If you don't need or use telemetry, you can instead use the IMPLEMENT_TRACELOGGING_CLASS_WITHOUT_TELEMETRY.
// This prevents telemetry from enabling your provider even if you're not using telemetry.

#define IMPLEMENT_TRACELOGGING_CLASS(TraceLoggingClassName, ProviderName, ProviderId) \
    __IMPLEMENT_TRACELOGGING_CLASS(TraceLoggingClassName, ProviderName, ProviderId)
#define IMPLEMENT_TRACELOGGING_CLASS_WITHOUT_TELEMETRY(TraceLoggingClassName, ProviderName, ProviderId) \
    __IMPLEMENT_TRACELOGGING_CLASS_WITHOUT_TELEMETRY(TraceLoggingClassName, ProviderName, ProviderId)

// [Optional] Externally using a Tracelogging class
// Use TraceLoggingProviderWrite to directly use the trace logging provider externally from the class in code.
// This is recommended only for simple TraceLogging events.  Telemetry events and activities are better defined
// within your Tracelogging class using one of the macros below.

#define TraceLoggingProviderWrite(TraceLoggingClassName, EventId, ...) \
    TraceLoggingWrite(TraceLoggingClassName::TraceLoggingType::Provider(), EventId, __VA_ARGS__)

#define TraceLoggingProviderWriteTelemetry(TraceLoggingClassName, EventId, ...) \
    TraceLoggingWrite(TraceLoggingClassName::TraceLoggingType::Provider(), EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY), __VA_ARGS__)

#define TraceLoggingProviderWriteMeasure(TraceLoggingClassName, EventId, ...) \
    TraceLoggingWrite(TraceLoggingClassName::TraceLoggingType::Provider(), EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES), __VA_ARGS__)

#define TraceLoggingProviderWriteCriticalData(TraceLoggingClassName, EventId, ...) \
    TraceLoggingWrite(TraceLoggingClassName::TraceLoggingType::Provider(), EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA), __VA_ARGS__)


// [Optional] Custom Events
// Use these macros to define a Custom Event for a Provider.  Use the TraceLoggingClassWrite or TraceLoggingClassWriteTelemetry
// from within a cusotm event to issue the event.  Methods will be a no-op (and not be called) if the provider is not
// enabled.

#define TraceLoggingClassWrite(EventId, ...) \
    TraceLoggingWrite(TraceLoggingType::Provider(), EventId, __VA_ARGS__)

#define TraceLoggingClassWriteTelemetry(EventId, ...) \
    TraceLoggingWrite(TraceLoggingType::Provider(), EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY), __VA_ARGS__)

#define TraceLoggingClassWriteMeasure(EventId, ...) \
    TraceLoggingWrite(TraceLoggingType::Provider(), EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES), __VA_ARGS__)

#define TraceLoggingClassWriteCriticalData(EventId, ...) \
    TraceLoggingWrite(TraceLoggingType::Provider(), EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA), __VA_ARGS__)

#define DEFINE_EVENT_METHOD(MethodName) \
    template<typename... TArgs> \
    static void MethodName(TArgs&&... args) \
    { \
        if (IsEnabled()) \
        { Instance()->MethodName##_(wistd::forward<TArgs>(args)...); } \
    } \
    void MethodName##_


// [Optional] Simple Events
// Use these macros to define very simple telemetry events for a Provider.  The events can
// be TELEMETRY events or TRACELOGGING events.

#define DEFINE_TELEMETRY_EVENT(EventId) \
    DEFINE_TRACELOGGING_EVENT(EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TELEMETRY_EVENT_PARAM1(EventId, VarType1, varName1) \
    DEFINE_TRACELOGGING_EVENT_PARAM1(EventId, VarType1, varName1, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TELEMETRY_EVENT_PARAM2(EventId, VarType1, varName1, VarType2, varName2) \
    DEFINE_TRACELOGGING_EVENT_PARAM2(EventId, VarType1, varName1, VarType2, varName2, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TELEMETRY_EVENT_PARAM3(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3) \
    DEFINE_TRACELOGGING_EVENT_PARAM3(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TELEMETRY_EVENT_PARAM4(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4) \
    DEFINE_TRACELOGGING_EVENT_PARAM4(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TELEMETRY_EVENT_PARAM5(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5) \
    DEFINE_TRACELOGGING_EVENT_PARAM5(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TELEMETRY_EVENT_PARAM6(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6) \
    DEFINE_TRACELOGGING_EVENT_PARAM6(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TELEMETRY_EVENT_PARAM7(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7) \
    DEFINE_TRACELOGGING_EVENT_PARAM7(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TELEMETRY_EVENT_PARAM8(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, VarType8, varName8) \
        DEFINE_TRACELOGGING_EVENT_PARAM8(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, VarType8, varName8, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))

#define DEFINE_TELEMETRY_EVENT_UINT32(EventId, varName)  DEFINE_TELEMETRY_EVENT_PARAM1(EventId, UINT32, varName)
#define DEFINE_TELEMETRY_EVENT_BOOL(EventId, varName)    DEFINE_TELEMETRY_EVENT_PARAM1(EventId, bool, varName)
#define DEFINE_TELEMETRY_EVENT_STRING(EventId, varName)  DEFINE_TELEMETRY_EVENT_PARAM1(EventId, PCWSTR, varName)

// [Optional] Simple Events
// Use these macros to define very simple measure events for a Provider.

#define DEFINE_MEASURES_EVENT(EventId) \
    DEFINE_TRACELOGGING_EVENT(EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_MEASURES_EVENT_PARAM1(EventId, VarType1, varName1) \
    DEFINE_TRACELOGGING_EVENT_PARAM1(EventId, VarType1, varName1, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_MEASURES_EVENT_PARAM2(EventId, VarType1, varName1, VarType2, varName2) \
    DEFINE_TRACELOGGING_EVENT_PARAM2(EventId, VarType1, varName1, VarType2, varName2, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_MEASURES_EVENT_PARAM3(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3) \
    DEFINE_TRACELOGGING_EVENT_PARAM3(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_MEASURES_EVENT_PARAM4(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4) \
    DEFINE_TRACELOGGING_EVENT_PARAM4(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_MEASURES_EVENT_PARAM5(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5) \
    DEFINE_TRACELOGGING_EVENT_PARAM5(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_MEASURES_EVENT_PARAM6(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6) \
    DEFINE_TRACELOGGING_EVENT_PARAM6(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_MEASURES_EVENT_PARAM7(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7) \
    DEFINE_TRACELOGGING_EVENT_PARAM7(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_MEASURES_EVENT_PARAM8(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, VarType8, varName8) \
        DEFINE_TRACELOGGING_EVENT_PARAM8(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, VarType8, varName8, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))

#define DEFINE_MEASURES_EVENT_UINT32(EventId, varName)  DEFINE_MEASURES_EVENT_PARAM1(EventId, UINT32, varName)
#define DEFINE_MEASURES_EVENT_BOOL(EventId, varName)    DEFINE_MEASURES_EVENT_PARAM1(EventId, bool, varName)
#define DEFINE_MEASURES_EVENT_STRING(EventId, varName)  DEFINE_MEASURES_EVENT_PARAM1(EventId, PCWSTR, varName)

// [Optional] Simple Events
// Use these macros to define very simple critical data events for a Provider.

#define DEFINE_CRITICAL_DATA_EVENT(EventId) \
    DEFINE_TRACELOGGING_EVENT(EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_CRITICAL_DATA_EVENT_PARAM1(EventId, VarType1, varName1) \
    DEFINE_TRACELOGGING_EVENT_PARAM1(EventId, VarType1, varName1, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_CRITICAL_DATA_EVENT_PARAM2(EventId, VarType1, varName1, VarType2, varName2) \
    DEFINE_TRACELOGGING_EVENT_PARAM2(EventId, VarType1, varName1, VarType2, varName2, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_CRITICAL_DATA_EVENT_PARAM3(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3) \
    DEFINE_TRACELOGGING_EVENT_PARAM3(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_CRITICAL_DATA_EVENT_PARAM4(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4) \
    DEFINE_TRACELOGGING_EVENT_PARAM4(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_CRITICAL_DATA_EVENT_PARAM5(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5) \
    DEFINE_TRACELOGGING_EVENT_PARAM5(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_CRITICAL_DATA_EVENT_PARAM6(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6) \
    DEFINE_TRACELOGGING_EVENT_PARAM6(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_CRITICAL_DATA_EVENT_PARAM7(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7) \
    DEFINE_TRACELOGGING_EVENT_PARAM7(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_CRITICAL_DATA_EVENT_PARAM8(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, VarType8, varName8) \
        DEFINE_TRACELOGGING_EVENT_PARAM8(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, VarType8, varName8, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))

#define DEFINE_CRITICAL_DATA_EVENT_UINT32(EventId, varName)  DEFINE_CRITICAL_DATA_EVENT_PARAM1(EventId, UINT32, varName)
#define DEFINE_CRITICAL_DATA_EVENT_BOOL(EventId, varName)    DEFINE_CRITICAL_DATA_EVENT_PARAM1(EventId, bool, varName)
#define DEFINE_CRITICAL_DATA_EVENT_STRING(EventId, varName)  DEFINE_CRITICAL_DATA_EVENT_PARAM1(EventId, PCWSTR, varName)


// Custom Activities
// For these you pair the appropriate BEGIN and END macros to define your activity.  Within the pair
// you can use the (TODO: LIST MACRO NAMES) macros to add behavior.

// [optional] params are:  Options, Keyword, Level
#define BEGIN_CUSTOM_ACTIVITY_CLASS(ActivityClassName, ...)                     __BEGIN_TRACELOGGING_ACTIVITY_CLASS(ActivityClassName, __VA_ARGS__) \
                                                                                __IMPLEMENT_ACTIVITY_CLASS(ActivityClassName)

// [optional] param is: Level
#define BEGIN_TRACELOGGING_ACTIVITY_CLASS(ActivityClassName)                    __BEGIN_TRACELOGGING_ACTIVITY_CLASS(ActivityClassName) \
                                                                                __IMPLEMENT_ACTIVITY_CLASS(ActivityClassName)
#define BEGIN_TRACELOGGING_ACTIVITY_CLASS_WITH_LEVEL(ActivityClassName, Level)  __BEGIN_TRACELOGGING_ACTIVITY_CLASS(ActivityClassName, wil::ActivityOptions::None, 0, Level) \
                                                                                __IMPLEMENT_ACTIVITY_CLASS(ActivityClassName)

// [optional] param is: Level
#define BEGIN_CALLCONTEXT_ACTIVITY_CLASS(ActivityClassName)                     __BEGIN_TRACELOGGING_ACTIVITY_CLASS(ActivityClassName, wil::ActivityOptions::TelemetryOnFailure) \
                                                                                __IMPLEMENT_ACTIVITY_CLASS(ActivityClassName)
#define BEGIN_CALLCONTEXT_ACTIVITY_CLASS_WITH_LEVEL(ActivityClassName, Level)   __BEGIN_TRACELOGGING_ACTIVITY_CLASS(ActivityClassName, wil::ActivityOptions::TelemetryOnFailure, 0, Level) \
                                                                                __IMPLEMENT_ACTIVITY_CLASS(ActivityClassName)

// [optional] param is: Level
#define BEGIN_TELEMETRY_ACTIVITY_CLASS(ActivityClassName)                       __BEGIN_TRACELOGGING_ACTIVITY_CLASS(ActivityClassName, wil::ActivityOptions::TelemetryOnFailure, MICROSOFT_KEYWORD_TELEMETRY) \
                                                                                __IMPLEMENT_ACTIVITY_CLASS(ActivityClassName)
#define BEGIN_TELEMETRY_ACTIVITY_CLASS_WITH_LEVEL(ActivityClassName, Level)     __BEGIN_TRACELOGGING_ACTIVITY_CLASS(ActivityClassName, wil::ActivityOptions::TelemetryOnFailure, MICROSOFT_KEYWORD_TELEMETRY, Level) \
                                                                                __IMPLEMENT_ACTIVITY_CLASS(ActivityClassName)

// [optional] param is: Level
#define BEGIN_MEASURES_ACTIVITY_CLASS(ActivityClassName)                       __BEGIN_TRACELOGGING_ACTIVITY_CLASS(ActivityClassName, wil::ActivityOptions::TelemetryOnFailure, MICROSOFT_KEYWORD_MEASURES) \
                                                                                __IMPLEMENT_ACTIVITY_CLASS(ActivityClassName)
#define BEGIN_MEASURES_ACTIVITY_CLASS_WITH_LEVEL(ActivityClassName, Level)     __BEGIN_TRACELOGGING_ACTIVITY_CLASS(ActivityClassName, wil::ActivityOptions::TelemetryOnFailure, MICROSOFT_KEYWORD_MEASURES, Level) \
                                                                                __IMPLEMENT_ACTIVITY_CLASS(ActivityClassName)

// [optional] param is: Level
#define BEGIN_CRITICAL_DATA_ACTIVITY_CLASS(ActivityClassName)                       __BEGIN_TRACELOGGING_ACTIVITY_CLASS(ActivityClassName, wil::ActivityOptions::TelemetryOnFailure, MICROSOFT_KEYWORD_CRITICAL_DATA) \
                                                                                __IMPLEMENT_ACTIVITY_CLASS(ActivityClassName)
#define BEGIN_CRITICAL_DATA_ACTIVITY_CLASS_WITH_LEVEL(ActivityClassName, Level)     __BEGIN_TRACELOGGING_ACTIVITY_CLASS(ActivityClassName, wil::ActivityOptions::TelemetryOnFailure, MICROSOFT_KEYWORD_CRITICAL_DATA, Level) \
                                                                                __IMPLEMENT_ACTIVITY_CLASS(ActivityClassName)

// Use to end ALL activity class definitions
#define END_ACTIVITY_CLASS()                                                    __END_TRACELOGGING_ACTIVITY_CLASS()


// Simple Activities
// For these you just use the appropriate macro to define the KIND of activity you want and specify
// the name (for tracelogging you can give other options)

// [optional] params are:  Options, Keyword, Level
#define DEFINE_CUSTOM_ACTIVITY(ActivityClassName, ...) \
    BEGIN_CUSTOM_ACTIVITY_CLASS(ActivityClassName, __VA_ARGS__) \
    END_ACTIVITY_CLASS()

#define DEFINE_TRACELOGGING_ACTIVITY(ActivityClassName) \
    BEGIN_TRACELOGGING_ACTIVITY_CLASS(ActivityClassName) \
    END_ACTIVITY_CLASS()
#define DEFINE_TRACELOGGING_ACTIVITY_WITH_LEVEL(ActivityClassName, Level) \
    BEGIN_TRACELOGGING_ACTIVITY_CLASS_WITH_LEVEL(ActivityClassName, Level) \
    END_ACTIVITY_CLASS()

#define DEFINE_CALLCONTEXT_ACTIVITY(ActivityClassName) \
    BEGIN_CALLCONTEXT_ACTIVITY_CLASS(ActivityClassName) \
    END_ACTIVITY_CLASS()
#define DEFINE_CALLCONTEXT_ACTIVITY_WITH_LEVEL(ActivityClassName, Level) \
    BEGIN_CALLCONTEXT_ACTIVITY_CLASS_WITH_LEVEL(ActivityClassName, Level) \
    END_ACTIVITY_CLASS()

#define DEFINE_TELEMETRY_ACTIVITY(ActivityClassName) \
    BEGIN_TELEMETRY_ACTIVITY_CLASS(ActivityClassName) \
    END_ACTIVITY_CLASS()
#define DEFINE_TELEMETRY_ACTIVITY_WITH_LEVEL(ActivityClassName, Level) \
    BEGIN_TELEMETRY_ACTIVITY_CLASS_WITH_LEVEL(ActivityClassName, Level) \
    END_ACTIVITY_CLASS()

#define DEFINE_MEASURES_ACTIVITY(ActivityClassName) \
    BEGIN_MEASURES_ACTIVITY_CLASS(ActivityClassName) \
    END_ACTIVITY_CLASS()
#define DEFINE_MEASURES_ACTIVITY_WITH_LEVEL(ActivityClassName, Level) \
    BEGIN_MEASURES_ACTIVITY_CLASS_WITH_LEVEL(ActivityClassName, Level) \
    END_ACTIVITY_CLASS()

#define DEFINE_CRITICAL_DATA_ACTIVITY(ActivityClassName) \
    BEGIN_CRITICAL_DATA_ACTIVITY_CLASS(ActivityClassName) \
    END_ACTIVITY_CLASS()
#define DEFINE_CRITICAL_DATA_ACTIVITY_WITH_LEVEL(ActivityClassName, Level) \
    BEGIN_CRITICAL_DATA_ACTIVITY_CLASS_WITH_LEVEL(ActivityClassName, Level) \
    END_ACTIVITY_CLASS()


// [Optional] Custom Start or Stop Events for Activities
// Use these macros to define cusotm start or custom stop methods for an activity.  Any activity can
// have multiple start or stop methods.  To add cusotm start or stop events, define a StartActivity instance
// method or a Stop instance method within the BEGIN/END pair of a cusotm activity.  Within that function, use
// TraceLoggingClassWriteStart or TraceLoggingClassWriteStop.

// Params:  (EventId, ...)
#define TraceLoggingClassWriteStart __WRITE_ACTIVITY_START
#define TraceLoggingClassWriteStop  __WRITE_ACTIVITY_STOP


// [Optional] Custom Tagged Events for Activities
// Use these macros to define a Custom Tagged Event for a Custom Activity.  Use the
// TraceLoggingClassWriteTagged or TraceLoggingClassWriteTaggedTelemetry macros from within a custom event
// to write the event.

#define TraceLoggingClassWriteTagged(EventId, ...) \
    __WI_TraceLoggingWriteTagged(*this, #EventId, __VA_ARGS__)

#define TraceLoggingClassWriteTaggedTelemetry(EventId, ...) \
    __WI_TraceLoggingWriteTagged(*this, #EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY), __VA_ARGS__)

#define TraceLoggingClassWriteTaggedMeasure(EventId, ...) \
    __WI_TraceLoggingWriteTagged(*this, #EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES), __VA_ARGS__)

#define TraceLoggingClassWriteTaggedCriticalData(EventId, ...) \
    __WI_TraceLoggingWriteTagged(*this, #EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA), __VA_ARGS__)

// [Optional] Simple Tagged Events for Activities
// Use these methods to define very simple tagged events for a Custom Activity.

#define DEFINE_TAGGED_TELEMETRY_EVENT(EventId) \
    DEFINE_TAGGED_TRACELOGGING_EVENT(EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TAGGED_TELEMETRY_EVENT_PARAM1(EventId, VarType1, varName1) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(EventId, VarType1, varName1, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TAGGED_TELEMETRY_EVENT_PARAM2(EventId, VarType1, varName1, VarType2, varName2) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(EventId, VarType1, varName1, VarType2, varName2, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TAGGED_TELEMETRY_EVENT_PARAM3(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM3(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TAGGED_TELEMETRY_EVENT_PARAM4(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM4(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TAGGED_TELEMETRY_EVENT_PARAM5(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM5(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TAGGED_TELEMETRY_EVENT_PARAM6(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM6(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TAGGED_TELEMETRY_EVENT_PARAM7(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM7(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))
#define DEFINE_TAGGED_TELEMETRY_EVENT_PARAM8(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, VarType8, varName8) \
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM8(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, VarType8, varName8, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY))

#define DEFINE_TAGGED_TELEMETRY_EVENT_UINT32(EventId, varName)  DEFINE_TAGGED_TELEMETRY_EVENT_PARAM1(EventId, UINT32, varName)
#define DEFINE_TAGGED_TELEMETRY_EVENT_BOOL(EventId, varName)    DEFINE_TAGGED_TELEMETRY_EVENT_PARAM1(EventId, bool, varName)
#define DEFINE_TAGGED_TELEMETRY_EVENT_STRING(EventId, varName)  DEFINE_TAGGED_TELEMETRY_EVENT_PARAM1(EventId, PCWSTR, varName)

// [Optional] Simple Tagged Events for Activities
// Use these methods to define very simple tagged measures events for a Custom Activity.

#define DEFINE_TAGGED_MEASURES_EVENT(EventId) \
    DEFINE_TAGGED_TRACELOGGING_EVENT(EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_TAGGED_MEASURES_EVENT_PARAM1(EventId, VarType1, varName1) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(EventId, VarType1, varName1, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_TAGGED_MEASURES_EVENT_PARAM2(EventId, VarType1, varName1, VarType2, varName2) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(EventId, VarType1, varName1, VarType2, varName2, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_TAGGED_MEASURES_EVENT_PARAM3(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM3(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_TAGGED_MEASURES_EVENT_PARAM4(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM4(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_TAGGED_MEASURES_EVENT_PARAM5(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM5(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_TAGGED_MEASURES_EVENT_PARAM6(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM6(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_TAGGED_MEASURES_EVENT_PARAM7(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM7(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))
#define DEFINE_TAGGED_MEASURES_EVENT_PARAM8(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, VarType8, varName8) \
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM8(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, VarType8, varName8, TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES))

#define DEFINE_TAGGED_MEASURES_EVENT_UINT32(EventId, varName)  DEFINE_TAGGED_MEASURES_EVENT_PARAM1(EventId, UINT32, varName)
#define DEFINE_TAGGED_MEASURES_EVENT_BOOL(EventId, varName)    DEFINE_TAGGED_MEASURES_EVENT_PARAM1(EventId, bool, varName)
#define DEFINE_TAGGED_MEASURES_EVENT_STRING(EventId, varName)  DEFINE_TAGGED_MEASURES_EVENT_PARAM1(EventId, PCWSTR, varName)

// [Optional] Simple Tagged Events for Activities
// Use these methods to define very simple tagged CRITICAL_DATA events for a Custom Activity.

#define DEFINE_TAGGED_CRITICAL_DATA_EVENT(EventId) \
    DEFINE_TAGGED_TRACELOGGING_EVENT(EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_TAGGED_CRITICAL_DATA_EVENT_PARAM1(EventId, VarType1, varName1) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1(EventId, VarType1, varName1, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_TAGGED_CRITICAL_DATA_EVENT_PARAM2(EventId, VarType1, varName1, VarType2, varName2) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2(EventId, VarType1, varName1, VarType2, varName2, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_TAGGED_CRITICAL_DATA_EVENT_PARAM3(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM3(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_TAGGED_CRITICAL_DATA_EVENT_PARAM4(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM4(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_TAGGED_CRITICAL_DATA_EVENT_PARAM5(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM5(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_TAGGED_CRITICAL_DATA_EVENT_PARAM6(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM6(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_TAGGED_CRITICAL_DATA_EVENT_PARAM7(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7) \
    DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM7(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))
#define DEFINE_TAGGED_CRITICAL_DATA_EVENT_PARAM8(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, VarType8, varName8) \
        DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM8(EventId, VarType1, varName1, VarType2, varName2, VarType3, varName3, VarType4, varName4, VarType5, varName5, VarType6, varName6, VarType7, varName7, VarType8, varName8, TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA))

#define DEFINE_TAGGED_CRITICAL_DATA_EVENT_UINT32(EventId, varName)  DEFINE_TAGGED_CRITICAL_DATA_EVENT_PARAM1(EventId, UINT32, varName)
#define DEFINE_TAGGED_CRITICAL_DATA_EVENT_BOOL(EventId, varName)    DEFINE_TAGGED_CRITICAL_DATA_EVENT_PARAM1(EventId, bool, varName)
#define DEFINE_TAGGED_CRITICAL_DATA_EVENT_STRING(EventId, varName)  DEFINE_TAGGED_CRITICAL_DATA_EVENT_PARAM1(EventId, PCWSTR, varName)


// Thread Activities [deprecated]
// These are desktop only and are not recommended by the fundamentals team.  These activities lag behind regular activities in
// their ability to use CallContext or to be cross-thread portable, so their usage should be limited.

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define BEGIN_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS_WITH_KEYWORD_LEVEL(ActivityClassName, keyword, level) \
    class ActivityClassName final : public _TlgActivityBase<ActivityClassName, keyword, level> \
    { \
        friend class _TlgActivityBase<ActivityClassName, keyword, level>; \
        void OnStarted() { PushThreadActivityId(); } \
        void OnStopped() { PopThreadActivityId(); } \
    public: \
        ActivityClassName() : m_result(S_OK) \
        { \
        } \
    private: \
        template<typename... TArgs> \
        ActivityClassName(_In_ void **, TArgs&&... args) : m_result(S_OK) \
        { \
            StartActivity(wistd::forward<TArgs>(args)...); \
        } \
    protected: \
        void EnsureWatchingCurrentThread() {} \
        void IgnoreCurrentThread() {} \
        wil::FailureInfo const *GetFailureInfo() \
        { \
            return (FAILED(m_result) && (m_cache.GetFailure() != nullptr) && (m_result == m_cache.GetFailure()->hr)) ? m_cache.GetFailure() : nullptr; \
        } \
        HRESULT GetResult() \
        { \
            return m_result; \
        } \
    public: \
        ~ActivityClassName() \
        { \
            Stop(HRESULT_FROM_WIN32(ERROR_UNHANDLED_EXCEPTION)); \
        } \
        ActivityClassName(const ActivityClassName &) = default; \
        ActivityClassName(ActivityClassName &&) = default; \
        TraceLoggingHProvider Provider() const \
        { \
            return TraceLoggingType::Provider(); \
        } \
        void Stop(HRESULT hr = S_OK) \
        { \
            if (IsStarted()) \
            { \
                m_result = hr; \
                TRACELOGGING_WRITE_ACTIVITY_STOP(ActivityClassName); \
            } \
        } \
        template<typename... TArgs> \
        void StopWithResult(HRESULT hr, TArgs&&... args) \
        { \
            m_result = hr; \
            Stop(wistd::forward<TArgs>(args)...); \
        } \
        template<typename... TArgs> \
        static ActivityClassName Start(TArgs&&... args) \
        { \
            return ActivityClassName(static_cast<void **>(__nullptr), wistd::forward<TArgs>(args)...); \
        } \
        void StartActivity() \
        { \
            TRACELOGGING_WRITE_ACTIVITY_START(ActivityClassName); \
        } \

#define BEGIN_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS_WITH_KEYWORD(ActivityClassName, keyword) \
    BEGIN_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS_WITH_KEYWORD_LEVEL(ActivityClassName, keyword, WINEVENT_LEVEL_VERBOSE)

#define BEGIN_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS_WITH_LEVEL(ActivityClassName, level) \
    BEGIN_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS_WITH_KEYWORD_LEVEL(ActivityClassName, 0, level)

#define BEGIN_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS(ActivityClassName) \
    BEGIN_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS_WITH_KEYWORD_LEVEL(ActivityClassName, 0, WINEVENT_LEVEL_VERBOSE)

#define END_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS() \
    private: \
        HRESULT m_result; \
        wil::ThreadFailureCache m_cache; \
    };

#define BEGIN_DEFINE_TELEMETRY_THREAD_ACTIVITY_CLASS(ActivityClassName) \
    BEGIN_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS_WITH_KEYWORD(ActivityClassName, MICROSOFT_KEYWORD_TELEMETRY)

#define END_DEFINE_TELEMETRY_THREAD_ACTIVITY_CLASS() \
    END_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS()

#define DEFINE_TRACELOGGING_THREAD_ACTIVITY_WITH_KEYWORD_LEVEL(ActivityClassName, keyword, level) \
    BEGIN_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS_WITH_KEYWORD_LEVEL(ActivityClassName, keyword, level) \
    END_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS()

#define DEFINE_TRACELOGGING_THREAD_ACTIVITY_WITH_KEYWORD(ActivityClassName, keyword) \
    BEGIN_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS_WITH_KEYWORD(ActivityClassName, keyword) \
    END_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS()

#define DEFINE_TRACELOGGING_THREAD_ACTIVITY_WITH_LEVEL(ActivityClassName, level) \
    BEGIN_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS_WITH_LEVEL(ActivityClassName, level) \
    END_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS()

#define DEFINE_TRACELOGGING_THREAD_ACTIVITY(ActivityClassName) \
    BEGIN_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS(ActivityClassName) \
    END_DEFINE_TRACELOGGING_THREAD_ACTIVITY_CLASS()

#define DEFINE_TELEMETRY_THREAD_ACTIVITY(ActivityClassName) \
    BEGIN_DEFINE_TELEMETRY_THREAD_ACTIVITY_CLASS(ActivityClassName) \
    END_DEFINE_TELEMETRY_THREAD_ACTIVITY_CLASS()

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */


// [deprecated]
// DO NOT USE these concepts
// These should be removed post RI/FI cycle...

#define DEFINE_TRACELOGGING_METHOD                  DEFINE_EVENT_METHOD
#define BEGIN_DEFINE_TELEMETRY_ACTIVITY_CLASS       BEGIN_TELEMETRY_ACTIVITY_CLASS
#define END_DEFINE_TELEMETRY_ACTIVITY_CLASS         END_ACTIVITY_CLASS
#define BEGIN_DEFINE_TRACELOGGING_ACTIVITY_CLASS    BEGIN_TRACELOGGING_ACTIVITY_CLASS
#define END_DEFINE_TRACELOGGING_ACTIVITY_CLASS      END_ACTIVITY_CLASS
#define TELEMETRY_WRITE_ACTIVITY_START              TraceLoggingClassWriteStart
#define TRACELOGGING_WRITE_ACTIVITY_START           TraceLoggingClassWriteStart
#define TELEMETRY_WRITE_ACTIVITY_STOP               TraceLoggingClassWriteStop
#define TRACELOGGING_WRITE_ACTIVITY_STOP            TraceLoggingClassWriteStop
#define WRITE_TRACELOGGING_EVENT                    TraceLoggingClassWrite
#define WRITE_TELEMETRY_EVENT                       TraceLoggingClassWriteTelemetry
#define TRACELOGGING_WRITE_TAGGED_EVENT             TraceLoggingClassWriteTagged
#define TELEMETRY_WRITE_TAGGED_EVENT                TraceLoggingClassWriteTaggedTelemetry

// [deprecated]
// DO NOT USE these concepts
// These should be removed post RI/FI cycle...
#define __DEFINE_EVENT                              DEFINE_TRACELOGGING_EVENT
#define __DEFINE_EVENT_PARAM1                       DEFINE_TRACELOGGING_EVENT_PARAM1
#define __DEFINE_EVENT_PARAM2                       DEFINE_TRACELOGGING_EVENT_PARAM2
#define __DEFINE_EVENT_PARAM3                       DEFINE_TRACELOGGING_EVENT_PARAM3
#define __DEFINE_EVENT_PARAM4                       DEFINE_TRACELOGGING_EVENT_PARAM4
#define __DEFINE_EVENT_PARAM5                       DEFINE_TRACELOGGING_EVENT_PARAM5
#define __DEFINE_EVENT_PARAM6                       DEFINE_TRACELOGGING_EVENT_PARAM6
#define __DEFINE_EVENT_PARAM7                       DEFINE_TRACELOGGING_EVENT_PARAM7
#define __DEFINE_EVENT_UINT32                       DEFINE_TRACELOGGING_EVENT_UINT32
#define __DEFINE_EVENT_BOOL                         DEFINE_TRACELOGGING_EVENT_BOOL
#define __DEFINE_EVENT_STRING                       DEFINE_TRACELOGGING_EVENT_STRING

// [deprecated]
// DO NOT USE these concepts
// These should be removed post RI/FI cycle...
#define __DEFINE_TAGGED_EVENT                       DEFINE_TAGGED_TRACELOGGING_EVENT
#define __DEFINE_TAGGED_EVENT_PARAM1                DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM1
#define __DEFINE_TAGGED_EVENT_PARAM2                DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM2
#define __DEFINE_TAGGED_EVENT_PARAM3                DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM3
#define __DEFINE_TAGGED_EVENT_PARAM4                DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM4
#define __DEFINE_TAGGED_EVENT_PARAM5                DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM5
#define __DEFINE_TAGGED_EVENT_PARAM6                DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM6
#define __DEFINE_TAGGED_EVENT_PARAM7                DEFINE_TAGGED_TRACELOGGING_EVENT_PARAM7
#define __DEFINE_TAGGED_EVENT_UINT32                DEFINE_TAGGED_TRACELOGGING_EVENT_UINT32
#define __DEFINE_TAGGED_EVENT_BOOL                  DEFINE_TAGGED_TRACELOGGING_EVENT_BOOL
#define __DEFINE_TAGGED_EVENT_STRING                DEFINE_TAGGED_TRACELOGGING_EVENT_STRING

template <typename T>
class ActivityErrorTracer
{
public:
    ActivityErrorTracer(T const &) {}
};

using TelemetryBase = wil::TraceLoggingProvider;

#define TRACELOGGING_WRITE_EVENT(TraceLoggingClassName, EventId, ...) \
    TraceLoggingWrite(TraceLoggingClassName::TraceLoggingType::Provider(), EventId, __VA_ARGS__)

#define TELEMETRY_WRITE_EVENT(EventId, ...) \
    TraceLoggingWrite(TraceLoggingType::Provider(), EventId, TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY), __VA_ARGS__)

#define DEFINE_TAGGED_EVENT_METHOD(MethodName) \
    public: void MethodName

#define DEFINE_ACTIVITY_START(...) \
    void StartActivity(__VA_ARGS__)

#define DEFINE_ACTIVITY_STOP(...) \
    void Stop(__VA_ARGS__)

#define DECLARE_TRACELOGGING_CLASS(TraceLoggingClassName, ProviderName, ProviderId) \
    class TraceLoggingClassName : public wil::TraceLoggingProvider \
    { \
    IMPLEMENT_TRACELOGGING_CLASS(TraceLoggingClassName, ProviderName, ProviderId); \
    };

#define IMPLEMENT_TELEMETRY_CLASS(TelemetryClassName, TraceLoggingClassName) \
    __IMPLEMENT_TRACELOGGING_CLASS_BASE(TelemetryClassName, TraceLoggingClassName) \
    protected: \
        void Create() \
        { AttachProvider(TraceLoggingClassName::Provider()); \
          __TRACELOGGING_DEFINE_PROVIDER_STORAGE_LINK(TelemetryClassName, TraceLoggingClassName); } \
    public:

namespace wil
{
    /// @cond
    namespace details
    {
        #ifdef WIL_API_TELEMETRY_SUSPEND_HANDLER
        #pragma detect_mismatch("ODR_violation_WIL_API_TELEMETRY_SUSPEND_HANDLER_mismatch", "1")
        #else
        #pragma detect_mismatch("ODR_violation_WIL_API_TELEMETRY_SUSPEND_HANDLER_mismatch", "0")
        #endif
        
        class ApiTelemetryLogger : public wil::TraceLoggingProvider
        {
        IMPLEMENT_TRACELOGGING_CLASS(ApiTelemetryLogger, "Microsoft.Windows.ApiTelemetry", (0xfb7fcbc6,0x7156,0x5a5b,0xea,0xbd,0x0b,0xe4,0x7b,0x14,0xf4,0x53)); 

        public:
            // Used to store of list of APIs (with namespace, class, custom and call count data per API).
            // This is public so that it can be unit tested. 
            class ApiDataList
            {
            public:
                struct ApiData
                {
                    PCWSTR className = nullptr;
                    PCWSTR apiName = nullptr;
                    PCSTR specialization = nullptr;
                    volatile long* counterReference = nullptr;
                    wistd::unique_ptr<ApiData> next;
            
                    ApiData(PCWSTR className_, PCWSTR apiName_, PCSTR specialization_, volatile long* counterReference_) :
                        className(className_), apiName(apiName_), specialization(specialization_), counterReference(counterReference_)
                    {
                    }
                };
            
                // Inserts a new Api call counter into the list, keeping the list sorted by className
                void Insert(PCWSTR className, PCWSTR apiName, _In_opt_ PCSTR specialization, volatile long* counterReference)
                {
                    wistd::unique_ptr<ApiData> newApiData(new(m_noThrow) ApiData(className, apiName, specialization, counterReference));
                    if (newApiData)
                    {
                        auto lock = m_lock.lock_exclusive(); 
                        
                        // Insert the new ApiData, keeping the list sorted by className.  
                        wistd::unique_ptr<ApiData>* currentNode = &m_root;
                        while (*currentNode)
                        {
                            wistd::unique_ptr<ApiData>& node = *currentNode;
                            if (wcscmp(className, node->className) <= 0)
                            {
                                break;
                            }
                            currentNode = &(node->next);
                        }
                        newApiData->next.reset(currentNode->release());
                        currentNode->reset(newApiData.release());
                    }
                }
            
                // For each distinct namespace, calls the provided flushCallback function.
                // After returning, it will have deleted all ApiData elements, and zeroed the *counterReference stored in each ApiData.
                void Flush(wistd::function<void(PCWSTR, PCWSTR, PCSTR, UINT32*, UINT16)> flushCallback)
                {   
                    wistd::unique_ptr<ApiData> root;
                    if (m_root)
                    {
                        auto lock = m_lock.lock_exclusive();
                        root.swap(m_root);                   
                    }
                   
                    while (root)
                    {   
                        // First find the number of characters we need to allocate for each string, and the number of items in the counter array to allocate
                        size_t totalApiListLength = 1; // Init to 1 to account for null terminator 
                        size_t totalSpecializationsLength = 1; // Init to 1 to account for null terminator 
                        UINT16 numCounts = 0;
                        
                        ProcessSingleNamespace(&root,
                            [&](wistd::unique_ptr<ApiData>& node)
                            {
                                 // Get the length needed for the class string
                                const wchar_t* strAfterNamespace = GetClassStringPointer(node->className);
                                size_t classStrLen = wcslen(strAfterNamespace ? strAfterNamespace : node->className);
                
                                totalApiListLength += (classStrLen + wcslen(node->apiName) + 1); // We add 1 to account for the comma delimeter
                                if (node->specialization)
                                {
                                    totalSpecializationsLength += strlen(node->specialization) + 1; // We add 1 to account for the comma delimeter
                                }
                                else
                                {
                                    totalSpecializationsLength += 2; // '-' plus comma delimeter
                                }
                                numCounts++;                               
                            });
            
                        // Fill arrays with the API data, and then pass it to the callback function              
                        wistd::unique_ptr<wchar_t[]> apiList(new(m_noThrow) wchar_t[totalApiListLength]);
                        wistd::unique_ptr<char[]> specializationList(new(m_noThrow) char[totalSpecializationsLength]);
                        wistd::unique_ptr<UINT32[]> countArray(new(m_noThrow) UINT32[numCounts]);
                        size_t nameSpaceLength = GetNameSpaceLength(root->className) + 1;
                        wistd::unique_ptr<wchar_t[]> nameSpace(new(m_noThrow) wchar_t[nameSpaceLength]);            
                        if (!apiList || !specializationList || !countArray || !nameSpace)
                        {
                            return; 
                        }

                        ZeroMemory(apiList.get(), totalApiListLength * sizeof(wchar_t));
                        ZeroMemory(specializationList.get(), totalSpecializationsLength * sizeof(char));
                        ZeroMemory(countArray.get(), numCounts * sizeof(UINT32));
                        ZeroMemory(nameSpace.get(), nameSpaceLength * sizeof(wchar_t));

                        StringCchCopyNW(nameSpace.get(), STRSAFE_MAX_CCH, root->className, nameSpaceLength - 1);
                        
                        int countArrayIndex = 0;

                        wistd::unique_ptr<ApiData>* lastNamespaceNode = ProcessSingleNamespace(&root,
                            [&](wistd::unique_ptr<ApiData>& node)
                            {
                                countArray[countArrayIndex] = static_cast<UINT32>(::InterlockedExchangeNoFence(node->counterReference, 0));
                                
                                // Prepend the portion of the apiName group string that's after the '.'. So for example, if the 
                                // className is "Windows.System.Launcher", then we prepend "Launcher." to the apiName string. 
                                const wchar_t* strAfterNamespace = GetClassStringPointer(node->className);
                                if (strAfterNamespace)
                                {
                                    FAIL_FAST_IF_FAILED(StringCchCatW(apiList.get(), totalApiListLength, strAfterNamespace + 1));
                                    FAIL_FAST_IF_FAILED(StringCchCatW(apiList.get(), totalApiListLength, L"."));
                                }
                
                                FAIL_FAST_IF_FAILED(StringCchCatW(apiList.get(), totalApiListLength, node->apiName));
                                if (node->specialization)
                                {
                                    FAIL_FAST_IF_WIN32_ERROR(strncat_s(specializationList.get(), totalSpecializationsLength, node->specialization, strlen(node->specialization)) != 0);                                 
                                }
                                else
                                {
                                    FAIL_FAST_IF_WIN32_ERROR(strncat_s(specializationList.get(), totalSpecializationsLength, "-", 1) != 0);  
                                }
                
                                if (countArrayIndex != (numCounts - 1))
                                {
                                    FAIL_FAST_IF_FAILED(StringCchCatW(apiList.get(), totalApiListLength, L","));
                                    FAIL_FAST_IF_WIN32_ERROR(strncat_s(specializationList.get(), totalSpecializationsLength, ",", 1) != 0);
                                }

                                countArrayIndex++;                                                              
                            });        

                        // Call the callback function with the data we've collected for this namespace
                        flushCallback(nameSpace.get(), apiList.get(), specializationList.get(), countArray.get(), numCounts);

                        if (*lastNamespaceNode)
                        {
                            root.swap((*lastNamespaceNode)->next);
                        }
                        else
                        {
                            root.reset();
                        }
                    }
                }
            
            private:
                static wistd::unique_ptr<ApiData>* ProcessSingleNamespace(wistd::unique_ptr<ApiData>* root, wistd::function<void(wistd::unique_ptr<ApiData>&)> workerCallback)
                {
                    wistd::unique_ptr<ApiData>* currentNode = root;
                    while (*currentNode)
                    {   
                        wistd::unique_ptr<ApiData>& node = *currentNode;
                        
                        workerCallback(node);
        
                        // Check if our next node would be a new namespace; if so, then break out
                        if (node->next && !IsSameNameSpace(node->className, node->next->className))
                        {
                            break;
                        }
                        
                        currentNode = &(node->next);
                    }                    

                    return currentNode; 
                }
                
                static bool IsSameNameSpace(PCWSTR namespaceClass1, PCWSTR namespaceClass2)
                {
                    return (wcsncmp(namespaceClass1, namespaceClass2, GetNameSpaceLength(namespaceClass2) + 1) == 0);
                }
            
                static size_t GetNameSpaceLength(PCWSTR nameSpaceClass)
                {
                    const wchar_t* strAfterNamespace = GetClassStringPointer(nameSpaceClass);
                    return (strAfterNamespace ? (strAfterNamespace - nameSpaceClass) : wcslen(nameSpaceClass));
                }

                static const wchar_t* GetClassStringPointer(PCWSTR nameSpaceClass)
                {
                    // Note: Usage of wcsrchr can cause build errors in some components, so we implement a way of getting the pointer to the 'class' portion
                    // of the string ourselves. 
                    int retIndex = 0;
                    while (nameSpaceClass[retIndex] != '\0')
                    {
                        retIndex++;
                    }
                    while (retIndex > 0 && nameSpaceClass[retIndex] != '.')
                    {
                        retIndex--;
                    }
                    return (retIndex != 0 ? &(nameSpaceClass[retIndex]) : nullptr);
                }
            
                wistd::unique_ptr<ApiData> m_root;
                wil::srwlock m_lock;

                // We create our own struct for non-throwing new, because std::nothrow can result in a redefinition error
                struct std::nothrow_t m_noThrow;   
            };
            
        public:
            // Initializes an entry that holds the className.apiName, along with a counter for that className.apiName. 
            // The counterReference passed to this should later be passed to LogApiInfo. 
            //
            // A separate entry will be created for each apiName that has a distinct specialization value. 
            //
            // This function only needs to be called once for each API, although it doesn't hurt if it gets called more than once.
            //
            // The apiName, className, and specialization parameters should be compile time constants. specialization can be null.
            DEFINE_EVENT_METHOD(InitApiData)(PCWSTR className, PCWSTR apiName, _In_opt_ PCSTR specialization, volatile long* counterReference)
            {   
                // TODO: Validate that apiName and className are a compile-time constants; validate that specialization is
                // either compile-time constant or nullptr; validate that counterReference points to static variable.  
                // Can do this by making sure address is <= (GetModuleHandle() + DLL size). 
                m_apiDataList.Insert(className, apiName, specialization, counterReference);          
            }

            // Fires a telemetry event that contains the method call apiName that has been logged by the component,
            // since the last FireEvent() call, or since the component was loaded. 
            DEFINE_EVENT_METHOD(FireEvent)()
            {      
                m_apiDataList.Flush(
                    [](PCWSTR nameSpace, PCWSTR apiList, PCSTR specializationList, UINT32* countArray, UINT16 numCounters)
                    {
                        TraceLoggingWrite(Provider(), "ApiCallCounts", TraceLoggingValue(nameSpace, "Namespace"), TraceLoggingValue(apiList, "ApiDataList"),
                           TraceLoggingValue(specializationList, "CustomList"), TraceLoggingUInt32Array(countArray, numCounters, "HitCounts"), TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
                           TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));     

                        __TRACELOGGING_TEST_HOOK_VERIFY_API_TELEMETRY(nameSpace, apiList, specializationList, countArray, numCounters);
                    });
            
                if (m_fireEventDelay < c_fireEventDelayLimit)
                {   
                    // Double the exponential backoff timer, until it reaches the maximum
                    m_fireEventDelay *= 2;
                    if (m_fireEventDelay > c_fireEventDelayLimit)
                    {
                        m_fireEventDelay = c_fireEventDelayLimit;
                    }     
                }

                ScheduleFireEventCallback();
            }
   
        private:
            void Initialize() WI_NOEXCEPT override 
            {   
#ifdef WIL_API_TELEMETRY_SUSPEND_HANDLER
                m_fireEventDelay = c_fireEventDelayLimit;

                PPSM_APPSTATE_REGISTRATION psmReg;
                BOOLEAN quiesced;
                PsmRegisterAppStateChangeNotification( 
                    [](BOOLEAN quiesced, PVOID, HANDLE) 
                    { 
                        if (quiesced) 
                        {
                            FireEvent(); 
                        }
                    }, 
                    StateChangeCategoryApplication, 0, nullptr, &quiesced, &psmReg); 
#else
                m_fireEventDelay = __TRACELOGGING_TEST_HOOK_API_TELEMETRY_EVENT_DELAY_MS;
#endif
                m_fireEventThreadPoolTimer.reset(::CreateThreadpoolTimer(  
                    [](PTP_CALLBACK_INSTANCE, PVOID, PTP_TIMER)  
                    {  
                        FireEvent();    
                    },  
                    nullptr,  
                    nullptr));                                  
                ScheduleFireEventCallback();
            }
            
            ~ApiTelemetryLogger() WI_NOEXCEPT
            {
                FireEvent();                 

                // release handle to thread pool timer instead of its destructor being call, if process is being terminated and dll is not being unloaded dynamically
                // destruction of threadpool timer is considered invalid during process termination
				if (ProcessShutdownInProgress())
                {
                    m_fireEventThreadPoolTimer.release();
                }
            }

            void ScheduleFireEventCallback()
            {
                // do not schedule thread pool timer callback, if process is being terminated and dll is not being unloaded dynamically
                if (m_fireEventThreadPoolTimer && !ProcessShutdownInProgress())
                {
                    // Note this will override any pending scheduled callback
                    FILETIME dueTime;
                    *reinterpret_cast<PLONGLONG>(&dueTime) = -static_cast<LONGLONG>(m_fireEventDelay * 10000);                   
                    SetThreadpoolTimer(m_fireEventThreadPoolTimer.get(), &dueTime, 0, 0);
                }
            }     
            
            ApiDataList m_apiDataList;
            wil::unique_threadpool_timer m_fireEventThreadPoolTimer; 

            // The timer used to determine when to fire the next telemetry event (when it's fired based on a timer). 
            UINT m_fireEventDelay;             
            DWORD const c_fireEventDelayLimit = 20*60*1000; // 20 minutes
        };
    } // namespace details
    /// @endcond
} // namespace wil

// Insert WI_LOG_API_USE near the top of a WinRT method to log that a method was called. 
// The parameter should be the method name, for example:
//  - WI_LOG_API_USE(L"LaunchUriAsync");
//
// To log that the WinRT method reached a certain line of code, pass an override string:
//  - WI_LOG_API_USE(L"LaunchUriAsync", "PointA");
//
// If the class name can't be obtained at runtime, or if instrumenting a non-WinRT API, use the below macro,
// and pass the fully qualified class name (in the case of WinRT), or a string identifying the group of the non-WinRT API:
//  - WI_LOG_CLASS_API_USE(RuntimeClass_Windows_System_Launcher, L"LaunchUriAsync"); 
//
// Note: If the component can have a suspend handler, the following line should be added before including TraceLogging.h:
//  - #define WIL_API_TELEMETRY_SUSPEND_HANDLER
// This will optimize the component's ability to upload telemetry, as it will upload on suspend. It will also disable
// frequent telemetry upload early in process execution. 
//
// Note: In your DLLMain method, please also add following code snippet
//		
//		wil::details::g_processShutdownInProgress = (lpReserved == nullptr);
//
// Adding this code snippet ensures that during process termination, thread pool timer
// destructor or SetThreadPoolTimer methods are not called, because they are invalid to call
// when dll is not getting dynamically unloaded. Skipping this code block will result in a continuable
// exception being thrown if process is getting terminated and dll in which ApiTelemetryLogger is not getting dynamically
// unloaded. For more details about lpReserved parameter, please refer to MSDN. 

#define __WI_LOG_CLASS_API_USE3(className, apiName, specialization) \
    do \
    { \
        static volatile long __wil_apiCallCounter = 0; \
        if (1 == ::InterlockedIncrementNoFence(&__wil_apiCallCounter)) \
        { \
            ::wil::details::ApiTelemetryLogger::InitApiData(className, apiName, specialization, &__wil_apiCallCounter); \
        } \
    } \
    while (0,0)      
#define __WI_LOG_CLASS_API_USE2(className, apiName) \
    __WI_LOG_CLASS_API_USE3(className, apiName, nullptr)
#define __WI_LOG_API_USE2(apiName, specialization) \
    __WI_LOG_CLASS_API_USE3(InternalGetRuntimeClassName(), apiName, specialization)    
#define __WI_LOG_API_USE1(apiName) \
    __WI_LOG_CLASS_API_USE3(InternalGetRuntimeClassName(), apiName, nullptr)

#define WI_LOG_CLASS_API_USE(...) \
    WI_MACRO_DISPATCH(__WI_LOG_CLASS_API_USE, __VA_ARGS__)

#define WI_LOG_API_USE(...) \
    WI_MACRO_DISPATCH(__WI_LOG_API_USE, __VA_ARGS__)

#pragma warning(pop)
