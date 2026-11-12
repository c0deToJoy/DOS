# Distributed Operating Systems (CS 5/4113)
## Lab 1: Queuing Lab Part II — Comprehensive System Design & Architecture Analysis

---

## 1. Overview & Context

This document provides an exhaustive, slide-by-slide technical breakdown of the presentation deck for **Lab 1: Queuing Lab Part II** (CS 5/4113: Distributed Operating Systems).

While Part I of the lab evaluated queuing mechanisms in a discrete, synchronous, fixed-interval simulated environment, **Part II transitions the architecture into an asynchronous, pipeline-driven distributed systems model**. Processes run concurrently, communicate across boundaries via Linux Inter-Process Communication (IPC) primitives—specifically named pipes (`mkfifo`) and later Google Remote Procedure Calls (`gRPC`)—and generate asynchronous telemetry (event traces and metric snapshots) that must be aggregated, analyzed, and stress-tested at varying scales.

---

## 2. Slide-by-Slide Breakdown and Diagram Explanations

---

### Slide 1: Title Slide
* **Slide Heading:** Lab 1: Queuing Lab Part II
* **Course Identifier:** CS 5/4113
* **Purpose:** Introduces the second installment of the queuing systems lab series in Distributed Operating Systems, establishing the foundation for building, orchestrating, and analyzing non-blocking, multi-stage queuing systems.

---

### Slide 2: Simple Queueing System (Core Abstract Model)

#### Visual Diagram Layout & Components
The diagram illustrates the canonical queuing theory abstraction consisting of an entry edge, an internal storage buffer, a service station, and egress routing:

```
                  +-----------------------------------+
                  |        Queue (has max-size)       |
                  |                                   |
Incoming Edges    |   +---+---+---+---+---+---+---+   |      Server Station
from other queues ===>|   |   |   |   |   |   |   |===|===> (has #-of-servers)
                  |   +---+---+---+---+---+---+---+   |             |
Edge from source  ===>|   [Slot 1] ... [Slot N]   |   |             |  Outgoing Edges
(Workload file)   +-----------------------------------+             |  to other queues
                                                                    +===> ========>
                                                                    |
                                                                    +===> Edge to Sink
                                                                          (Output file)
```

#### Detailed Element Explanations
1. **Inputs:**
   * **Source Edge:** An incoming edge reading jobs from an external input file containing synthetic or recorded workload traces.
   * **Upstream Edges:** Incoming edges arriving from previous queuing stages within a broader multi-tier network (tandem/feedforward topology).
2. **Queue Storage:**
   * Characterized by a finite capacity parameter: `max-size`.
   * Enforces buffer limits; if an arrival arrives while the queue length equals `max-size`, buffer overflow policies (packet drops or errors) are triggered.
3. **Server Station:**
   * Represented by the circular node directly attached to the queue buffer.
   * Parameterized by `#-of-servers` ($c$), defining the number of parallel workers or execution threads servicing jobs dequeued from the buffer.
4. **Outputs:**
   * **Downstream Edges:** Outgoing communication channels delivering processed jobs to subsequent queuing stages.
   * **Sink Edge:** A terminal edge writing completed jobs to an output file or workload trace sink for response time and throughput verification.

---

### Slide 3: New Queueing System (Pipeline Composition via Named Pipes)

#### Conceptual Evolution
Slide 3 transitions the abstract queue model of Slide 2 into concrete OS processes executing concurrently in user space. The system decomposes into independent binaries connected via POSIX named pipes (FIFOs) located in the Linux virtual filesystem (`/tmp/` and `/dev/`). This decouples the generator, the buffer manager, and the service workers, preparing the codebase for subsequent migration to distributed network RPCs (`gRPC`).

#### Visual System Architecture & Process Wiring

```
[ ./job_generator ]
  --dist=pois,5
  --outfile=/tmp/fifo2
           |
           | (writes jobs)
           v
      [/tmp/fifo2]
           |
           | (reads jobs)
           v
    [ ./queue ]
      --infile=/tmp/fifo2
      --outfile=/tmp/fifo0
      --outfile=/tmp/fifo1
      --logfile=q0.log
      --qsize=7
       /        \
      / (fifo0)  \ (fifo1)
     v            v
[/tmp/fifo0]  [/tmp/fifo1]
     |            |
     v            v
[ ./sf_stateless ]         [ ./sf_stateless ]
  --infile=/tmp/fifo0        --infile=/tmp/fifo1
  --outfile=/dev/stdout      --outfile=/dev/stdout
  --logfile=s0q0.log         --logfile=s1q0.log
     |                          |
     +------------+-------------+
                  v
            [/dev/stdout]
```

#### Component & Command Line Syntax Breakdown
1. **Job Generator (`./job_generator`):**
   * **Command:** `./job_generator --outfile=/tmp/fifo2 --dist=pois,5 &`
   * **Role:** Background process generating discrete jobs according to a Poisson arrival process with an average rate parameter ($\lambda = 5$). Writes serialized job entries into `/tmp/fifo2`.
2. **Finite Queue Coordinator (`./queue`):**
   * **Command:** `./queue --outfile=/tmp/fifo0 --outfile=/tmp/fifo1 --infile=/tmp/fifo2 --logfile=q0.log --qsize=7 &`
   * **Role:** Reads incoming jobs from `/tmp/fifo2`. It maintains an internal buffer capped at `--qsize=7`. It load-balances or fans out dequeued tasks across two output FIFO streams: `/tmp/fifo0` and `/tmp/fifo1`. Internal queue state transitions and drops are logged to `q0.log`.
3. **Stateless Server Functions (`./sf_stateless`):**
   * **Server 0:** `./sf_stateless --outfile=/dev/stdout --infile=/tmp/fifo0 --logfile=s0q0.log &`
   * **Server 1:** `./sf_stateless --outfile=/dev/stdout --infile=/tmp/fifo1 --logfile=s1q0.log &`
   * **Role:** Two worker processes pulling jobs concurrently from `/tmp/fifo0` and `/tmp/fifo1`. They simulate execution latency and emit completed jobs directly to `/dev/stdout`, while logging server execution metrics to `s0q0.log` and `s1q0.log`.

---

### Slide 4: Telemetry, Observability, and Operational Metrics

#### System Telemetry Requirements
Slide 4 maintains the multi-process pipeline visual from Slide 3 and introduces the primary telemetry requirements for long-running workflows. In asynchronous systems, diagnosing performance regressions requires instrumenting both steady-state flow and extreme tail events:

#### The 5 Golden Monitoring Signals
1. **Throughput ($X$):** Completed jobs exiting the system per unit time step.
2. **Latency / Response Time ($T$):** Total elapsed time from a job's initial arrival until its final departure from a sink.
3. **Error Rate ($E$):** Drops per unit time caused by saturated queues (`QUEUE-FULL`) or misconfigured routing topologies (`NO-OUT-EDGE`).
4. **Saturation ($S$):** The degree of backlog; the number of jobs waiting in the queue buffer unable to immediately enter service.
5. **Utilization ($U$):** The percentage of time server worker threads are actively executing tasks versus idling.

#### Percentile Distributions & The USE Dashboard
To capture tail behavior and service degradation:
* Systems cannot rely solely on the mean/average, as outliers distort operational reality.
* Metrics must be segmented into percentiles:
  * **P10:** Tenth percentile (fast-path executions).
  * **Median (P50):** Typical performance midpoint.
  * **P90:** Ninetieth percentile (identifies tail latency and queuing delays).
* **USE Dashboard Specification:** Every queue must maintain an independent visualization line displaying **Utilization**, **Saturation**, and **Errors** across time intervals.

---

### Slide 5: Section A — Workload Generator Implementation

#### Workload Generator Architecture
Slide 5 displays an arrow pointing out of a generator glyph (triangle), symbolizing a directed flow of workload tokens injected into an IPC channel.

```
+------------+
|  Job Gen   |=========> (Injected Stream of Jobs into FIFO/File)
| (Triangle) |
+------------+
```

#### Algorithmic Design & Logic
The generator operates as an autonomous process that continuously samples inter-arrival durations from parameterized statistical distributions:

```python
# Pseudo-code implementation based on Slide 5
with open(outfn, 'a') as outfile:
    while True:
        # Inter-arrival delay sampled from specified distribution (e.g., Poisson/Exponential)
        inter_arrival_time = random.SOMEDISTRIBUTION(cli_params)
        sleep(inter_arrival_time)
        
        # Instantiate and serialize job with service duration (task wait)
        outfile.write(Job(task_wait=random.rand))
```

#### Key Engineering Takeaways:
1. **Sanity Check:** A simple dummy load generator is implemented first to validate process pipes and FIFO non-blocking flags.
2. **Customized Distributions:** Real-world applications require tailored distributions (Exponential/Poisson for memoryless web traffic, Pareto/Weibull for bursty data transfers).
3. **Decoupled Job Attributes:** Each serialized job contains self-describing task metadata, including the synthetic execution work/delay (`task_wait`) required at downstream server workers.

---

### Slide 6: Simple Queueing System — Queue Configuration Table

#### Configuration Schema
Slide 6 provides a configuration matrix to declaratively define the topology, resource capacities, and routing endpoints of each queuing node in the system.

#### Example Configuration Table

| Queue sys ID | Max Queue Size | # of servers | Server Function | Edges in & source | Edges out & sink |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **q0** | 8 | 1 | `Sub-10.py` | `in0.wl` | `out0.wl` |

#### Field Semantics:
* **`Queue sys ID` (`q0`):** Unique identifier assigned to the queuing instance for log tagging and tracing.
* **`Max Queue Size` (`8`):** The hard buffer capacity limit ($K$). Any arrivals exceeding 8 pending jobs trigger an error.
* **`# of servers` (`1`):** Number of worker instances ($c$) assigned to process jobs from `q0`.
* **`Server Function` (`Sub-10.py`):** The specific executable or Python worker script invoked to execute the job's payload.
* **`Edges in & source` (`in0.wl`):** Ingress channel designation (named pipe or input workload file).
* **`Edges out & sink` (`out0.wl`):** Egress channel designation (downstream pipe or terminal workload sink file).

---

### Slide 7: Sample Logs & Metrics Collection Architecture

#### Log Stream Analysis
Slide 7 displays a raw snippet of the timestamped execution log generated by queue process `Q_pid67628`:

```text
2025-11-11 06:25:01,387 Q_pid67628@unknown... INFO Started
2025-11-11 06:25:01,387 Q_pid67628@unknown... INFO max-q-size 10
2025-11-11 06:25:01,387 Q_pid67628@unknown... INFO metric-q-size 0
2025-11-11 06:25:01,388 Q_pid67628@unknown... INFO Consumer Running
2025-11-11 06:25:01,388 Q_pid67628@unknown... INFO Producer 0 Running
2025-11-11 06:25:01,388 Q_pid67628@unknown... INFO Producer 1 Running
2025-11-11 06:25:01,488 Q_pid67628@unknown... INFO metric-q-size 10
... [Continuous entries showing metric-q-size hovering at 9 and 10] ...
2025-11-11 06:25:03,797 Q_pid67628@unknown... INFO Producer 1 Done
2025-11-11 06:25:03,807 Q_pid67628@unknown... INFO Consumer 0: Done
```

#### Core Insights from the Raw Log:
1. **Asynchronous Ingestion:** Multiple producer threads/processes concurrently pump jobs into the queue while consumer routines pull them off.
2. **Sustained Buffer Saturation:** The log demonstrates the queue filling to capacity (`metric-q-size 10` where `max-q-size 10`), proving that arrival rate exceeds service rate ($\lambda > c \cdot \mu$), driving the queue to maximum capacity.
3. **Structured Event Logging:** Every lifecycle boundary (`Started`, `Consumer Running`, `metric-q-size`, `Producer Done`) is timestamped with millisecond precision to facilitate post-mortem trace reconstruction.
4. **Tooling Integration:** Telemetry is dumped asynchronously using standard Python `logging`. The slide lists integration guides for **Prometheus** (time-series scraping) and **Grafana** (dashboard visualization) to monitor production queues using industry-standard observability stacks.

---

### Slide 8: Queue Metrics Specification (`qmet.csv`)

#### Asynchronous Log Rectification
In synchronous simulations, time advances in uniform, discrete ticks. In this asynchronous lab, real clock time elapses non-uniformly. Consequently, students must either:
* Adapt downstream visualization scripts to digest asynchronous event streams directly, or
* Build a pre-processing rectification script to sample the asynchronous logs into discrete time steps.

#### Data Schema: `qmet.csv`
At each time step, each active queuing system exports a standardized CSV record:

```csv
time-step,queue-sys-id,num-jobs-in-queue,num-jobs-in-service,num-of-errors
```

#### Column Definitions:
* **`time-step`:** The discrete integer interval or timestamp of the sampling window.
* **`queue-sys-id`:** The unique identifier of the reporting queue system (e.g., `q0`).
* **`num-jobs-in-queue`:** The count of waiting jobs in the buffer (representing **Saturation** $S$ or queue length $L_q$).
* **`num-jobs-in-service`:** The count of jobs currently occupying servers (representing busy capacity $B$ or active utilization).
* **`num-of-errors`:** Cumulative or delta count of dropped jobs (due to queue overflow or missing egress routes).

---

### Slide 9: Event Logs & Trace Formatting

#### Structured Event Grammar
Slide 9 defines an explicit string-formatted grammar for logging every discrete state transition a job undergoes. By capturing uniform event tokens, developers can trace any job end-to-end or isolate systemic errors using command-line tools like `grep` and `awk`.

```
====================================================================================
                        JOB LIFECYCLE EVENT SPECIFICATION
====================================================================================
<job-id> IN <qsid> ARRIVE-VIA <edge>       -> Enters queueing system via edge (source/pipe)
<job-id> IN <qsid> DEPART-VIA <edge>       -> Exits queueing system via edge (sink/pipe)
<job-id> IN <qsid> ENTERS-SERVER <sid>     -> Leaves buffer and enters server <sid>
<job-id> IN <qsid> EXITS-SERVER <sid>      -> Finishes execution and leaves server <sid>
<job-id> IN <qsid> TASK-START <tid> <task> -> Execution of specific sub-task begins
<job-id> IN <qsid> TASK-STOP <tid> <task>  -> Execution of specific sub-task completes
<job-id> IN <qsid> ERROR <err-type>        -> Failure: QUEUE-FULL, NO-OUT-EDGE
====================================================================================
```

#### Diagnostic Utility
* **Tracing Errors:** To isolate dropped tasks, run:
  ```bash
  grep "ERROR" system.log | awk '{print $1}' | xargs -I {} grep {} system.log
  ```
  This extracts the full operational lifecycle of every failed job.
* **Implementation:** Integrated across queue managers and stateless/stateful workers via Python's standard `logger.info()`.

---

### Slide 10: Section A.XX — Sanity Check & Scalability Testing

#### Verification Goals
Before modeling complex topologies, the basic pipeline (generator $\to$ queue $\to$ server $\to$ sink) must be validated to ensure metrics align with theoretical expectations.

#### Experimental Scales

| Scale Tier | Queue Count | Job Volume | System Focus |
| :--- | :--- | :--- | :--- |
| **Small** | Tens ($10^1$) | 1,000 – 10,000 jobs | Baseline validation, debugging, trace verification |
| **Medium** | Hundreds ($10^2$) | 10,000 – 100,000 jobs | Multi-process pipe throughput and context switching |
| **Large** | Thousands ($10^3$) | 100,000 – 1,000,000 jobs | File descriptor limits, memory footprint, tail latencies |
| **Very Large** | Tens of Thousands ($10^4$) | 100,000 – 1,000,000 jobs | Extreme saturation, IPC overhead, kernel buffer contention |

#### Sensitivity Study Framework
At each scale tier, students perform controlled perturbations to measure system elasticity:
* **Resource Expansion:** Evaluate throughput and latency improvements when provisioning $+25\%$, $+50\%$, and $+100\%$ resources (scaling worker servers $c$, increasing queue buffer depth $K$, or adding parallel inter-queue links).
* **Resource Starvation:** Measure degradation when stripping $-10\%$, $-20\%$, and $-30\%$ resources.

---

### Slide 11: Section B.XX — Modeling & Real-World System Analysis

#### Applied Modeling Exercise
Slide 11 tasks students with selecting and implementing **approximately two real-world queuing systems** (e.g., e-commerce order checkout, microservice API gateway fan-out, cloud build pipelines, or hospital triage flows) and executing them across the four scale tiers (Small to Very Large).

#### Architectural Inquiry Framework:
1. **Mapping:** How does the physical or business process translate into queues, server pools, and network edges?
2. **Metrics:** Which golden signals matter most for this specific application (e.g., strict P99 latency limits for web requests vs. maximizing throughput for batch data processing)?
3. **Workload Modeling:** What distribution generates the workload, and does it reflect real-world user arrival patterns?
4. **Behavioral Analysis:** How do USE metrics, response-time curves, and throughput saturation thresholds respond as scale grows?
5. **Practical Takeaways:** What engineering recommendations would you provide to an architect building this production system in a cloud-native or containerized cluster?

---

### Slide 12: Blank Conclusion Slide
* **Visual:** Minimalist clean slate indicating the end of the instructional deck.
* **Function:** Serves as an open forum for project planning, architecture design decisions, and team delegation for the implementation of Lab 1 Part II.

---

## 3. High-Level Summary of Lab 1 Part II Objectives

```
+-----------------------------------------------------------------------------------+
|                            LAB 1 PART II ROADMAP                                  |
+-----------------------------------------------------------------------------------+
|  1. Inter-Process Communication (IPC)                                            |
|     - Replace synchronous simulation loops with asynchronous POSIX named pipes.   |
|     - Lay the structural foundation for subsequent gRPC network distribution.     |
|                                                                                   |
|  2. Full-Lifecycle Observability                                                 |
|     - Instrument components using Python's logging library.                       |
|     - Generate standardized `qmet.csv` snapshots and detailed event log grammars. |
|     - Surface USE metrics (Utilization, Saturation, Errors) alongside P10/P50/P90.|
|                                                                                   |
|  3. Multi-Tier Scaling & Sensitivity Analysis                                     |
|     - Execute small (10 queues) to very large (10,000 queues) workload runs.     |
|     - Conduct sensitivity studies (+100% to -30% resources) to quantify bottlenecks|
|     - Map theoretical queuing models to two real-world distributed architectures. |
+-----------------------------------------------------------------------------------+
```