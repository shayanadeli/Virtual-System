# Virtual System

A simulated Unix-like file system and shell, written in C++ from the ground up — including its own virtual disk with sector-based storage, a tree-structured directory hierarchy, and an interactive command interpreter. No calls to the host OS's file system; every operation (allocation, deletion, defragmentation) is implemented manually against the simulated disk.

## How It Works

On startup, the program asks for a disk size in megabytes and builds a virtual disk out of fixed-size 64-byte sectors. Files and folders are represented as nodes in a tree (`Node`), and every file's content is broken into 64-byte chunks and written across free sectors on the virtual disk — the same way a real file system allocates blocks. Deleting a file frees its sectors back to the pool; running `defrag` walks the whole tree and repacks every file's sectors so they sit contiguously, exactly like a real disk defragmentation.

Paths are resolved manually, supporting both absolute (`/home/docs`) and relative paths, as well as `.` and `..` — the interpreter walks the tree node by node rather than relying on any OS path-handling.

## Supported Commands

| Command | Description |
|---|---|
| `pwd` | Prints the current path |
| `ls [path]` | Lists the contents of the current directory, or the given path |
| `cd [path]` | Changes the current directory (defaults to `/home` with no argument) |
| `mkdir <path>` | Creates a new directory |
| `rm <path>` | Recursively deletes a file or directory and frees its sectors |
| `cp <src> <dest>` | Copies a file or directory (recursively) to a new location |
| `mv <src> <dest>` | Moves/renames a file or directory |
| `put <real_file> [virtual_dir]` | Imports a real file from the host machine into the virtual disk |
| `get <virtual_file>` | Exports a virtual file back out to the host machine |
| `defrag` | Repacks every file's sectors so they're stored contiguously |
| `exit` | Quits the program |

## Technical Highlights

- **Sector-based virtual storage** — the disk is modeled as a fixed number of 64-byte sectors; file content is split into sector-sized chunks and allocated/freed manually, rather than just storing strings in memory.
- **Tree-based file system** with full path resolution, including relative paths and `..` traversal.
- **Host-bridging commands** (`put` / `get`) that move real files in and out of the simulated disk.
- **Defragmentation**, implemented by collecting every file's content, wiping the disk, and reallocating sectors contiguously.
- Input validation (name checks, disk-full handling, missing-argument handling) on every command.

## Tech Stack

- **Language:** C++ (C++11 or later — the code uses `nullptr`, `auto`, and lambdas)
- **Dependencies:** none beyond the C++ standard library (`<iostream>`, `<vector>`, `<string>`, `<algorithm>`, `<fstream>`)

## Getting Started

### Prerequisites
A C++ compiler such as `g++` or MSVC.

### Build

```bash
git clone https://github.com/shayanadeli/Virtual-System.git
cd Virtual-System
g++ -std=c++11 main.cpp -o virtual-system
```

### Run

```bash
./virtual-system
```

Example session:

```
Enter Disk size (MB): 10
Disk installed: 10 Megabytes = 163840 sectors of 64 bytes

File system is ready!
You are in /home
Type 'exit' to quit

/home> mkdir docs
/home> cd docs
/home/docs> put notes.txt
/home/docs> ls
notes.txt
/home/docs> pwd
/home/docs
/home/docs> defrag
defrag: completed
```

## What I Learned

Building Virtual System meant working through problems that real file systems actually deal with: managing storage as fixed-size blocks instead of arbitrary strings, tracking and reclaiming free space, resolving nested paths without any help from the OS, and repacking fragmented storage on demand. It's a much closer look at how a file system works than just calling standard library functions.

## License

[MISSING INFORMATION: اگه لایسنس خاصی نمی‌خوای انتخاب کنی، MIT ساده‌ترین و رایج‌ترین گزینه برای پروژه‌های شخصیه.]

---

# سیستم مجازی (Virtual System)

یک فایل‌سیستم و شل شبیه‌سازی‌شده از یونیکس، کاملاً از پایه با C++ نوشته شده — شامل یک دیسک مجازی با ذخیره‌سازی سکتوری، یک ساختار درختی برای دایرکتوری‌ها، و یک مفسر دستور تعاملی. هیچ عملیاتی (تخصیص فضا، حذف، دیفرگمنت) به سیستم‌عامل واگذار نمی‌شه؛ همه‌چیز مستقیم روی دیسک شبیه‌سازی‌شده پیاده‌سازی شده.

## نحوه‌ی عملکرد

با اجرای برنامه، اندازه‌ی دیسک به مگابایت گرفته می‌شه و یک دیسک مجازی از سکتورهای ثابت ۶۴ بایتی ساخته می‌شه. فایل‌ها و پوشه‌ها به‌صورت گره‌هایی در یک درخت (`Node`) نمایش داده می‌شن، و محتوای هر فایل به قطعات ۶۴ بایتی تقسیم و روی سکتورهای آزاد دیسک نوشته می‌شه — دقیقاً همون کاری که یک فایل‌سیستم واقعی برای تخصیص بلاک انجام می‌ده. حذف یک فایل، سکتورهاش رو به استخر فضای آزاد برمی‌گردونه؛ اجرای `defrag` کل درخت رو پیمایش می‌کنه و سکتورهای هر فایل رو به‌صورت پیوسته بازچینی می‌کنه، دقیقاً مثل دیفرگمنت واقعی دیسک.

مسیرها به‌صورت دستی resolve می‌شن، هم مسیر مطلق (`/home/docs`) و هم نسبی، به‌همراه `.` و `..` — مفسر دستور، گره‌به‌گره درخت رو پیمایش می‌کنه، بدون تکیه به هیچ قابلیت مسیریابی سیستم‌عامل.

## دستورات پشتیبانی‌شده

| دستور | توضیح |
|---|---|
| `pwd` | نمایش مسیر فعلی |
| `ls [path]` | نمایش محتوای دایرکتوری فعلی یا مسیر داده‌شده |
| `cd [path]` | تغییر دایرکتوری فعلی (بدون آرگومان به `/home` می‌ره) |
| `mkdir <path>` | ساخت یک دایرکتوری جدید |
| `rm <path>` | حذف بازگشتی یک فایل یا پوشه و آزادسازی سکتورهاش |
| `cp <src> <dest>` | کپی یک فایل یا پوشه (به‌صورت بازگشتی) به مسیر جدید |
| `mv <src> <dest>` | جابه‌جایی یا تغییرنام فایل/پوشه |
| `put <real_file> [virtual_dir]` | وارد کردن یک فایل واقعی از سیستم میزبان به دیسک مجازی |
| `get <virtual_file>` | خروجی گرفتن از یک فایل مجازی به سیستم میزبان |
| `defrag` | بازچینی پیوسته‌ی سکتورهای همه‌ی فایل‌ها |
| `exit` | خروج از برنامه |

## نکات فنی برجسته

- **ذخیره‌سازی سکتوری** — دیسک به‌صورت تعداد مشخصی سکتور ۶۴ بایتی مدل شده؛ محتوای فایل‌ها به قطعات سکتوری تقسیم و به‌صورت دستی تخصیص/آزاد می‌شن، نه صرفاً نگه‌داری رشته در حافظه.
- **فایل‌سیستم درختی** با resolve کامل مسیر، شامل مسیر نسبی و پیمایش `..`.
- **دستورات پل‌زننده به سیستم میزبان** (`put`/`get`) که فایل واقعی رو بین دیسک واقعی و دیسک مجازی جابه‌جا می‌کنن.
- **دیفرگمنت واقعی**، با جمع‌آوری محتوای همه‌ی فایل‌ها، پاک کردن دیسک، و تخصیص مجدد پیوسته‌ی سکتورها.
- اعتبارسنجی ورودی (بررسی نام، مدیریت پر بودن دیسک، مدیریت آرگومان ناقص) روی هر دستور.

## تکنولوژی

- **زبان:** C++ (C++11 یا بالاتر — کد از `nullptr`، `auto`، و lambda استفاده می‌کنه)
- **وابستگی:** فقط کتابخانه‌ی استاندارد C++ (`<iostream>`, `<vector>`, `<string>`, `<algorithm>`, `<fstream>`)، بدون هیچ کتابخانه‌ی خارجی

## شروع به کار

### پیش‌نیاز
یک کامپایلر C++ مثل `g++` یا MSVC.

### کامپایل

```bash
git clone https://github.com/shayanadeli/Virtual-System.git
cd Virtual-System
g++ -std=c++11 main.cpp -o virtual-system
```

### اجرا

```bash
./virtual-system
```

## چیزی که یاد گرفتم

ساخت این پروژه یعنی درگیر شدن با مسائلی که فایل‌سیستم‌های واقعی هم باهاشون سروکار دارن: مدیریت فضای ذخیره‌سازی به‌صورت بلاک‌های با اندازه‌ی ثابت به‌جای رشته‌های دلخواه، ردیابی و بازپس‌گیری فضای آزاد، resolve کردن مسیرهای تودرتو بدون کمک سیستم‌عامل، و بازچینی فضای پراکنده در صورت نیاز. این خیلی نزدیک‌تر به فهمیدن نحوه‌ی کارکرد یک فایل‌سیستم واقعیه تا صرفاً صدا زدن توابع کتابخانه‌ی استاندارد.

## لایسنس

[MISSING INFORMATION: اگه لایسنس خاصی نمی‌خوای انتخاب کنی، MIT ساده‌ترین و رایج‌ترین گزینه برای پروژه‌های شخصیه.]
