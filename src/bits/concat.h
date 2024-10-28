#pragma once

#define ____CONCAT_IMPL(a, b) a##b
#define ____CONCAT(a, b)      ____CONCAT_IMPL(a, b)