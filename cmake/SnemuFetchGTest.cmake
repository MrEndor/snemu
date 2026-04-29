include_guard(GLOBAL)

include(CPM)

CPMAddPackage(
        NAME googletest
        GITHUB_REPOSITORY google/googletest
        VERSION 1.17.0
        GIT_TAG v1.17.0
)
