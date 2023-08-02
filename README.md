# ranker-comparator


## Table of Contents

- [Requirements & Description](#Requirements & Description)
- [Installation](#installation)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)


## Requirements & Description

The goal of this project is to offer a basic set of tools to build and evaluate ranking algorithms. In order to achieve this we have implemented:

- A superclass named *ranker*, which includes some basic utilities to provide a quickstart for the development of new techniques.
- A top-k retrieval function to retrieve the top-k reated nodes of the network based on a min-hip for high efficiency.
- An implementation of the Jaccard Similarity Coefficient computation, which exploits the use of unordered sets for high efficiency.
- A multithreading library, which allows to parallelize the main (like we did) and also any possible ranker.
- A CSR matrix representation, to allow an efficient handling of the network matrices, also improved through the usage of buffers.

Also, 3 ranking algorithms are already implemented: Power-Method based PageRank, Power-Method based HITS, and InDegree.


## installation

In order to run this project a *c++ compiler* and *make* are required.

## Usage

To run the project using the *make* utility, the user needs to move in the directory containing the .hpp and .cpp files, and using the following commands: 

```bash
make        # To compile the project.
make exec   # To run the compiled project.
make clean  # To clean previous compiled versions of the project.
```


## Contributing

Can be easily expanded, and offers also a multithreading library for an easier handling of thread pools. 
The implemented barrier might need some further tweaks in order to be used inside of ranking algorithms (in order to avoid any deadlock).

```bash
git clone https://github.com/jgurakuqi/ranker-comparator
```


## License

MIT License

Copyright (c) 2023 Jurgen Gurakuqi, Elsa Sejdi

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS," WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
