#pragma once

#include "../vec.h"
#include "../protocol.h"
#include <stdbool.h>
#include <stdint.h>

struct InferredSessionParameters
{
    uint32_t seed;
    bool randomized_start;
    bool using_priority;
};

typedef struct InferredSessionParameters InferredSessionParameters;

struct InferredRequestParameters
{
    uint64_t start;
    uint64_t difficulty;
};

typedef struct InferredRequestParameters InferredRequestParameters;

struct AnalysisRequestEntry
{
    uint64_t id;
    ProtocolRequest req;
    InferredRequestParameters params;
};

typedef struct AnalysisRequestEntry AnalysisRequestEntry;

struct AnalysisRequestStorage
{
    InferredSessionParameters session_params;
    Vec(AnalysisRequestEntry) entries;
};

typedef struct AnalysisRequestStorage AnalysisRequestStorage;

void analysis_infer_request_parameters(const ProtocolRequest* req, InferredRequestParameters* params);

AnalysisRequestStorage* analysis_storage_create(uint32_t cap);
void analysis_storage_push(AnalysisRequestStorage* storage, const ProtocolRequest* req);
