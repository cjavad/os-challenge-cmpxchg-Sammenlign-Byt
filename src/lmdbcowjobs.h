https://cdn.discordapp.com/attachments/1302974338814705684/1302974469676990464/BambuClient.log?ex=672a1109&is=6728bf89&hm=d4d350aba64f8362f63a5e91988f96d11b6d190a82129fa972bb744d44889e12&#pragma once

// Implementation of CoW storage for jobs inspired by https://github.com/LMDB/lmdb /
// https://en.wikipedia.org/wiki/Lightning_Memory-Mapped_Database.
//
// We operate on a linked list based data structure where a unit of jobs is stored in a page.
// reads and writes happen on a per-page basis and an atomic compare-and-swap operation is used
// to swap out the page when the writer is done.
//
// Since we operate under a single-writer architecture we do not need to worry about retrying the write
// if we are contended with.

// Underlying data we store.
struct _ActualJob {};

struct CowJob {
    struct _ActualJob data;
};

struct CowJobPage {
    struct CowJobPage* next;
    struct _ActualJob jobs[0];
};