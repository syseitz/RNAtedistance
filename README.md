# RNA Distance Matrix Program

This programme calculates the tree edit distance between RNA secondary structures provided in dot-bracket notation and outputs a distance matrix. It is designed to be efficient and utilises parallel processing to make the most of available computational resources.

## Motivation of this program

During my Master Thesis I experienced RNAdistance to be very slow and not solve my problems in reasonable time. Therefore I have implemented my own version to calculate the Tree Edit Distance, to make 

## Compilation Instructions

The programme is written in C and can be compiled on Linux, Mac, and Windows systems. Ensure you have a C compiler installed on your system (e.g., GCC for Linux and Mac, MinGW for Windows).

### Linux

1. Open a terminal.
2. Navigate to the directory containing the source code.
3. Run the following command:
   ```bash
   gcc -o RNAtedistance -fopenmp RNAtedistance.c -O3
   ```
   This will compile the programme with OpenMP support for parallel processing.

### Mac

1. Open a terminal.
2. Navigate to the directory containing the source code.
3. Run the following command:
   ```bash
   clang -o RNAtedistance -fopenmp RNAtedistance.c -O3
   ```
   Note: You may need to install OpenMP support for Clang. If you encounter issues, consider using GCC via Homebrew (`brew install gcc`).

### Windows

1. Install MinGW or a similar C compiler for Windows.
2. Open a command prompt.
3. Navigate to the directory containing the source code.
4. Run the following command:
   ```cmd
   gcc -o RNAtedistance -fopenmp RNAtedistance.c -O3
   ```
   Ensure that your compiler supports OpenMP. If not, you may need to install a version that does or adjust the compilation flags.

## Usage Instructions

Once compiled, you can run the programme from the command line. The programme reads RNA secondary structures in dot-bracket notation from standard input, one per line, and outputs a distance matrix to standard output.

### Running the Programme

1. Open a terminal or command prompt.
2. Navigate to the directory containing the compiled programme.
3. Run the programme:
   - On Linux/Mac: `./RNAtedistance`
   - On Windows: `RNAtedistance.exe`
4. Enter RNA structures, one per line. Press Enter after each structure. To finish input, press Enter on an empty line.
5. The programme will compute the tree edit distances and display a progress indicator. Upon completion, it will output the distance matrix.

### Example

**Input:**
```
.((..))
((..))
```

**Output:**
```
0 1
1 0
```

### Options

The programme supports the following command-line options:

- `--help, -h`: Display help message.
- `--version, -v`: Display version information.
- `--threads, -t <number>`: Set the number of threads for parallel processing (default: all available cores).
- `--row-wise, -r`: Compute and output the distance matrix row by row (memory-efficient mode).

Example:
```bash
./RNAtedistance --threads 4 --row-wise
```

## Performance Facts

The programme offers two modes of operation: **Full Matrix Mode** and **Row-Wise Mode**. Below is a comparison of these modes with `RNAdistance`, based on execution time and memory usage for processing a set of RNA structures.

### System Specifications

The performance measurements were conducted on the following system:
- **Processor**: 13th Gen Intel(R) Core(TM) i7-13700 (16 cores, 24 threads)
- **Operating System**: Linux (x86_64)
- **Compiler**: GCC with OpenMP support

These specifications are based on the system used to test the programme with 1,000 structures.

### Comparison Table

| Mode                          | Execution Time (user time) | Total Time (elapsed) | Memory Usage (maxresident) | CPU Usage |
|-------------------------------|----------------------------|----------------------|----------------------------|-----------|
| **RNAtedistance (Full Matrix Mode)** | 410.60s                    | 17.22s               | 16,312 KB                  | 2384%     |
| **RNAtedistance (Row-Wise Mode)**    | 976.65s                    | 41.37s               | 12,460 KB                  | 2360%     |
| **RNAdistance**                      | 206.91s                    | 207.13s              | 4,224 KB                   | 99%       |

- **Execution Time (user time)**: Time spent in user mode.
- **Total Time (elapsed)**: Real time from start to finish of the programme.
- **Memory Usage (maxresident)**: Maximum resident set size (memory used).
- **CPU Usage**: Percentage of CPU time used.

### Observations

- **Full Matrix Mode**:
  - Faster in terms of total elapsed time (17.22s) due to efficient parallel processing.
  - Higher memory usage (16,312 KB) as the entire matrix is stored in memory.
  - Suitable for systems with sufficient RAM.

- **Row-Wise Mode**:
  - Slower in terms of total elapsed time (41.37s) due to frequent I/O operations.
  - Lower memory usage (12,460 KB) as only one row is computed and printed at a time.
  - Ideal for systems with limited RAM.

- **RNAdistance**:
  - Slower in terms of total elapsed time (207.13s) as it does not use parallel processing.
  - Lowest memory usage (4,224 KB), making it highly memory-efficient.
  - Suitable for environments where parallel processing is not available or when memory is a critical constraint.

### Memory Complexity and Requirements

- **Full Matrix Mode**:
  - **Memory Complexity**: O(n²), where n is the number of RNA structures.
  - The entire distance matrix is stored in memory, requiring significant RAM for large n.

- **Row-Wise Mode**:
  - **Memory Complexity**: O(n), as only one row of the matrix is stored at a time.
  - More memory-efficient, especially for large n, but with a trade-off in computation time due to increased I/O operations.

- **RNAdistance**:
  - **Memory Complexity**: O(n), similar to Row-Wise Mode, but with a different implementation that is single-threaded and optimised for memory usage.

#### Estimated Minimum RAM Requirements

The table below estimates the minimum RAM required for different numbers of structures in **Full Matrix Mode**, **Row-Wise Mode**, and **RNAdistance**. The estimates assume each distance is stored as a 4-byte integer.

| Number of Structures (n) | Full Matrix Mode (O(n²)) | Row-Wise Mode (O(n)) | RNAdistance (O(n)) |
|--------------------------|--------------------------|----------------------|--------------------|
| 1,000                    | ~4 MB                    | ~4 KB                | ~4 KB              |
| 10,000                   | ~400 MB                  | ~40 KB               | ~40 KB             |
| 100,000                  | ~40 GB                   | ~400 KB              | ~400 KB            |
| 1,000,000                | ~4 TB                    | ~4 MB                | ~4 MB              |

- **Full Matrix Mode**: Requires substantial RAM for large n (e.g., 40 GB for 100,000 structures).
- **Row-Wise Mode**: Remains memory-efficient even for very large n (e.g., 4 MB for 1,000,000 structures).
- **RNAdistance**: Matches Row-Wise Mode in memory efficiency due to its O(n) complexity.

### Recommendations

- **Use Full Matrix Mode**:
  - When sufficient RAM is available (e.g., 32 GB is sufficient for up to ~10,000 structures).
  - For faster computation times, leveraging multiple CPU cores.

- **Use Row-Wise Mode**:
  - When RAM is limited.
  - When memory efficiency is prioritised over speed.

- **Use RNAdistance**:
  - In environments where parallel processing is not possible.
  - When memory usage needs to be minimised.

### Conclusion

The choice of mode depends on the specific constraints and resources of your system:
- **Full Matrix Mode** offers speed at the cost of higher memory usage.
- **Row-Wise Mode** provides a memory-efficient alternative with a slower computation time.
- **RNAdistance** is a good fallback for single-threaded environments or when memory is extremely limited.

For most modern systems with multiple cores and adequate RAM, **Full Matrix Mode** is recommended for its superior performance.

## Understanding Tree Edit Distance for RNA Structures

RNA secondary structures can be represented as trees, where nodes correspond to paired or unpaired bases. The tree edit distance is a measure of similarity between two such trees, calculated as the minimum number of operations (insertions, deletions, and relabellings) required to transform one tree into another.

In this programme:
- **Nodes** are labelled as 'P' (pair) or 'U' (unpaired).
- **Operations**:
  - Inserting or deleting a 'P' node costs 2.
  - Inserting or deleting a 'U' node costs 1.
  - Relabelling a node costs 0 if the labels are the same, otherwise 1.

This distance metric provides a way to quantify structural differences between RNA molecules, which is useful in various bioinformatics applications.

## Notes

- Ensure that the input structures are valid dot-bracket notations with balanced parentheses.
- The programme assumes that the input structures are provided correctly; invalid inputs may lead to errors or crashes.
- For large numbers of structures or very long structures, the computation may take significant time due to the complexity of the tree edit distance algorithm.