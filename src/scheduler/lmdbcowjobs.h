// Implementation of CoW storage for jobs inspired by
// https://github.com/LMDB/lmdb /
// https://en.wikipedia.org/wiki/Lightning_Memory-Mapped_Database.
//
// We operate on a linked list based data structure where a unit of jobs is
// stored in a page. reads and writes happen on a per-page basis and an atomic
// compare-and-swap operation is used to swap out the page when the writer is
// done.
//
// Since we operate under a single-writer architecture we do not need to worry
// about retrying the write if we are contended with.

// Underlying data we store.
struct _ActualJob {};

struct CowJob {
    struct _ActualJob data;
};

struct CowJobPage {
    struct CowJobPage* next;
    struct _ActualJob jobs[0];
};