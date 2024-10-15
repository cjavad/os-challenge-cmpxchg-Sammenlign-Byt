use sha2::{Digest, Sha256};
use std::collections::{BTreeMap, HashMap};
use std::time::{Duration, Instant};

struct Entry {
    key: [u8; 32], // SHA-256 produces a 32-byte hash
    data: u64,
}

impl Entry {
    fn new(data: u64) -> Self {
        let mut hasher = Sha256::new();
        hasher.update(data.to_be_bytes());
        let result = hasher.finalize();

        let mut key = [0u8; 32];
        key.copy_from_slice(&result[..]);

        Self { key, data }
    }
}

struct Xorshift64 {
    state: u64,
}

impl Xorshift64 {
    fn new(seed: u64) -> Self {
        Self { state: seed }
    }

    fn next(&mut self) -> u64 {
        self.state ^= self.state << 23;
        self.state ^= self.state >> 17;
        self.state ^= self.state << 26;
        self.state
    }

    fn random_u64_in_range(&mut self, start: u64, end: u64) -> u64 {
        start + (self.next() % (end - start))
    }
}

fn benchmark_hashmap(seed: u64, num_entries: usize) {
    let mut rng = Xorshift64::new(seed);
    let mut hashmap: HashMap<[u8; 32], u64> = HashMap::with_capacity(num_entries);

    let start_insert = Instant::now();
    for _ in 0..num_entries {
        let data = rng.next();
        let entry = Entry::new(data);
        hashmap.insert(entry.key, entry.data);
    }
    let insert_duration = start_insert.elapsed();

    let start_retrieve = Instant::now();
    for _ in 0..num_entries {
        let data = rng.next();
        let entry = Entry::new(data);
        let value = hashmap.get(&entry.key);
    }
    let retrieve_duration = start_retrieve.elapsed();

    println!(
        "HashMap Insertion time: {:.2} ns/entry",
        insert_duration.as_nanos() as f64 / num_entries as f64
    );
    println!(
        "HashMap Retrieval time: {:.2} ns/entry",
        retrieve_duration.as_nanos() as f64 / num_entries as f64
    );
}

fn benchmark_btreemap(seed: u64, num_entries: usize) {
    let mut rng = Xorshift64::new(seed);
    let mut btreemap: BTreeMap<[u8; 32], u64> = BTreeMap::new();

    let start_insert = Instant::now();
    for _ in 0..num_entries {
        let data = rng.next();
        let entry = Entry::new(data);
        btreemap.insert(entry.key, entry.data);
    }
    let insert_duration = start_insert.elapsed();

    let start_retrieve = Instant::now();
    for _ in 0..num_entries {
        let data = rng.next();
        let entry = Entry::new(data);
        let value = btreemap.get(&entry.key);
    }
    let retrieve_duration = start_retrieve.elapsed();

    println!(
        "BTreeMap Insertion time: {:.2} ns/entry",
        insert_duration.as_nanos() as f64 / num_entries as f64
    );
    println!(
        "BTreeMap Retrieval time: {:.2} ns/entry",
        retrieve_duration.as_nanos() as f64 / num_entries as f64
    );
}

fn main() {
    let seed = 123456789; // Replace with custom seed
    let num_entries = 1_000_000;

    println!("Benchmarking HashMap:");
    benchmark_hashmap(seed, num_entries);

    println!("Benchmarking BTreeMap:");
    benchmark_btreemap(seed, num_entries);
}
