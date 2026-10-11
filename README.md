# ShopKart 🛒

**ShopKart** is a high-performance, Flipkart-style e-commerce platform built **EXCLUSIVELY in ISO C99, HTML5, and CSS3**.
There is **ZERO JavaScript, ZERO Python, ZERO Node.js, and ZERO external frameworks or dependencies**.

The single compiled C binary (`shopkart.exe`) acts as both the multithreaded-ready **HTTP web server** (using raw native Winsock2 on Windows and POSIX Berkeley sockets on Linux/macOS) and the **Data Structures & Algorithms (DSA) execution engine**.

---

## ⚡ Architecture & DSA Engine

ShopKart is engineered so that every real user-facing feature is powered by an explicit hand-crafted data structure or algorithm in pure C:

| Feature | Data Structure / Algorithm | File | Complexity (Time & Aux Space) | Stability |
| :--- | :--- | :--- | :--- | :--- |
| **Catalog Multi-Key Sorting** | **Merge Sort** | `src/sort.c` | $O(n \log n)$ time, $O(n)$ space | **Stable** (preserves secondary criteria) |
| **Alternative Sorting** | **Quick Sort** (Randomized Pivot) | `src/sort.c` | $O(n \log n)$ avg, $O(n^2)$ worst, $O(\log n)$ space | Unstable |
| **Price Range Filtering** | **Binary Search** (`lower_bound` / `upper_bound`) | `src/search.c` | $O(\log n + k)$ time, $O(1)$ space | N/A |
| **Direct Product Lookup** | **Binary Search (by ID)** | `src/search.c` | $O(\log n)$ time, $O(1)$ space | N/A |
| **Substring Product Search** | **Linear Substring Search** | `src/search.c` | $O(n \cdot m)$ time, $O(1)$ space | N/A |
| **Search Autocomplete** | **Trie (Prefix Tree)** | `src/search.c` | $O(p + s)$ time, $O(\Sigma \cdot \|V\|)$ space | N/A |
| **User & Session Caching** | **Hash Table (Separate Chaining)** | `src/hashmap.c` | $O(1)$ avg lookup/insert, $O(b + n)$ space | N/A |
| **Cart "Undo Remove"** | **Stack (LIFO)** | `src/stack.c` | $O(1)$ push/pop/peek, $O(k)$ space | N/A |
| **Recently Viewed History** | **Circular Queue (FIFO)** | `src/queue.c` | $O(1)$ enqueue/dequeue, $O(1)$ fixed 10-cap | N/A |
| **User Wishlist** | **Singly Linked List** | `src/linkedlist.c`| $O(1)$ append with tail, $O(n)$ remove/search | N/A |
| **Top 10 Deals / Ratings** | **Min/Max Binary Heap** | `src/heap.c` | $O(n \log k)$ time, $O(k)$ space | Unstable |
| **Price Hierarchy Index** | **Binary Search Tree (BST)** | `src/bst.c` | $O(h)$ insert, $O(n)$ in-order traversal | N/A |

---

## 🔒 Security & Educational Authentication Notice

> ⚠️ **Educational Note on Authentication:**
> Passwords in ShopKart are hashed using a hand-crafted implementation of **Salted SHA-256** in C (`src/auth.c`) with cryptographically secure salts (`CryptGenRandom` on Windows and `/dev/urandom` on POSIX).
> **This implementation is intended strictly for educational and demonstration purposes.**
> In a production application, memory-hard adaptive hashing algorithms such as **bcrypt**, **Argon2id**, or **scrypt** must always be used to protect against GPU-based offline dictionary attacks.

Key Safety Features in ShopKart C Codebase:
- **Zero Buffer Overflows:** Strictly no `gets`, `strcpy`, `sprintf`, or `strcat`. All operations use bounded `snprintf`, bounded `memcpy`, and explicit capacity checks.
- **Strict Size Limits:** HTTP request headers are capped at 8 KB; request bodies are capped at 16 KB.
- **Account Lockout:** Rate-limiting locks any account for 60 seconds after 5 consecutive failed login attempts.
- **Session Tokens:** Cryptographically random session tokens stored in hash table with 30-minute expiry, transmitted via `HttpOnly; SameSite=Lax` cookies.
- **Input Sanitization:** All user-supplied fields are HTML-escaped before server-side template rendering to prevent Cross-Site Scripting (XSS).
- **Directory Traversal Protection:** Static file router blocks any request containing `..`.

---

## 🛠️ Prerequisites & Installation on Windows 10/11

ShopKart requires **GCC** and **Make** (available via MSYS2 or MinGW).

### Step 1: Install MSYS2 (if not already installed)
1. Download the MSYS2 installer from [msys2.org](https://www.msys2.org/) or install via Windows Package Manager in PowerShell:
   ```powershell
   winget install MSYS2.MSYS2
   ```
2. Open the **MSYS2 UCRT64** terminal and install GCC, Make, and tools:
   ```bash
   pacman -Syu
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make
   ```
3. Add the MSYS2 MinGW binaries folder (e.g., `C:\msys64\ucrt64\bin`) to your Windows User or System `PATH` environment variable.
4. Verify GCC is accessible from PowerShell:
   ```powershell
   gcc --version
   ```

---

## 🚀 Building and Running ShopKart

### 1. Build the Executable
Open PowerShell in the `shopkart` directory:
```powershell
.\build.ps1 build
```
*(Alternative using Make: `make`)*

This compiles all 15 C modules with `-Wall -Wextra -O2 -std=c99` and links native Winsock2 (`-lws2_32`) with **zero warnings**.

### 2. Run Comprehensive DSA Unit Tests
To run the automated assert-based unit tests for all data structures:
```powershell
.\build.ps1 test
```
*(Alternative using Make: `make test`)*

### 3. Start the Web Server
Launch the server:
```powershell
.\shopkart.exe
```
The server will start listening at:
👉 **[http://localhost:8080](http://localhost:8080)**

---

## 🧭 Pre-seeded Demo Credentials

For quick evaluation, ShopKart includes a pre-seeded demo user with full shopping history:
- **Email:** `demo@shopkart.local`
- **Password:** `Demo@12345`

*Tip: The `/login` page features a **"⚡ Try Demo Account (One Click)"** button that pre-fills and submits the form instantly without bypassing standard authentication checks.*

---

## 🧪 Verification & Troubleshooting

### Run End-to-End Test Suite
Run the included verification script in PowerShell:
```powershell
powershell -ExecutionPolicy Bypass -File .\comprehensive_verify.ps1
```
This tests:
1. `GET /` — Home page rendering and animated CSS hero slider.
2. `GET /products` — Merge Sort vs Quick Sort execution and comparison tracking.
3. Binary search price range filtering verified against a naive CSV scan.
4. Product detail page specs table and similar products carousel.
5. Cookie-based authentication and 30-minute session expiry.
6. Private route redirects (`/wishlist`, `/checkout`, `/orders`).
7. Cart quantity adjustments, item removal, and Stack-based **"Undo Remove"**.
8. Address validation, payment options, and order placement.
9. Order persistence surviving server restarts (`data/orders.csv`).
10. Live DSA benchmarking at `/dsa?run=1&n=1000`.

### Troubleshooting Common Issues

1. **Port 8080 already in use:**
   ```powershell
   Get-NetTCPConnection -LocalPort 8080 -ErrorAction SilentlyContinue | Select-Object OwningProcess
   Stop-Process -Id <ProcessId> -Force
   ```
2. **GCC not recognized:**
   Ensure GCC is added to your Environment `PATH` (e.g., `C:\MinGW\bin` or `C:\msys64\ucrt64\bin`).
3. **Clean Rebuild:**
   ```powershell
   .\build.ps1 clean
   .\build.ps1 build
   ```

---

*Note: All products, prices, and ratings are demo data.*
