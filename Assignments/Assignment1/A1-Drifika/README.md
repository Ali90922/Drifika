]\documentclass[12pt]{article}
\usepackage[utf8]{inputenc}
\usepackage[T1]{fontenc}
\usepackage{lmodern}
\usepackage{hyperref}
\usepackage{listings}
\usepackage{xcolor}
\usepackage{enumitem}

% Define colors for code listings
\definecolor{codebg}{RGB}{245,245,245}
\definecolor{codeframe}{RGB}{220,220,220}

\lstset{
    backgroundcolor=\color{codebg},
    frame=single,
    rulecolor=\color{codeframe},
    basicstyle=\ttfamily,
    columns=fullflexible,
    breaklines=true
}

\title{COMP 3430 Assignment 1: exFAT File System Implementation}
\author{Your Name \\ COMP 3430, Winter 2025}
\date{\today}

\begin{document}

\maketitle

\section{Overview}
This project implements a read-only exFAT file system interface that mimics many aspects of the POSIX file system API. The goal of the assignment is to support basic file system operations on an exFAT-formatted disk image. The implementation includes functions to mount and unmount the file system, open files, read file data, list directory entries, and retrieve file sizes. In addition, a simple open file table (OFT) has been implemented to allow multiple files to be accessed simultaneously.

The interface defined in the header file \texttt{nqp\_io.h} is used throughout the code, while exFAT-specific types and constants are provided in \texttt{nqp\_exfat\_types.h}. A main interface (entry point) has been created to demonstrate and test the implemented functions.

\section{Features}

\subsection{Mounting \& Unmounting}
\begin{itemize}[noitemsep]
    \item \textbf{nqp\_mount:} Opens the file system image, reads the Main Boot Record (MBR), and performs a series of checks to ensure the file system is consistent. Critical fields such as the file system name (\texttt{"EXFAT   "}), boot signature (\texttt{0xAA55}), the \texttt{must\_be\_zero} block, and \texttt{first\_cluster\_of\_root\_directory} are validated.
    \item \textbf{nqp\_unmount:} Closes the file system image and resets the mounted state.
\end{itemize}

\subsection{File Operations}
\begin{itemize}[noitemsep]
    \item \textbf{nqp\_open:} Searches the directory structure (using a tokenized path) to locate and open a file. The file descriptor returned is the first cluster number of the file, and the file is recorded in an open file table.
    \item \textbf{nqp\_read:} Reads data from an open file by following its FAT chain. This function properly handles cases where file data spans multiple clusters.
    \item \textbf{nqp\_close:} Marks a file as closed (currently a placeholder since the open file table is simple).
    \item \textbf{nqp\_size:} Retrieves the size of a file by scanning directory entries for a matching first cluster.
\end{itemize}

\subsection{Directory Operations}
\begin{itemize}[noitemsep]
    \item \textbf{nqp\_getdents:} Reads directory entries one at a time. This function uses static variables to track progress through a directory, converts Unicode filenames to ASCII using \texttt{unicode2ascii}, and handles multi-cluster directories.
\end{itemize}

\subsection{Utilities}
\begin{itemize}[noitemsep]
    \item \textbf{unicode2ascii:} Converts 16-bit Unicode strings (with only ASCII characters) to regular 8-bit C strings.
    \item \textbf{print\_open\_file\_table:} (For debugging) Prints the current status of the open file table.
    \item \textbf{Main Interface:} A \texttt{main} interface has been created that demonstrates the functionality of the file system API. This interface is used by utility programs such as \texttt{cat}, \texttt{ls}, and \texttt{paste} to interact with the file system image.
\end{itemize}

\section{Compilation}
The project is designed to compile on the Aviary system. To compile the project, simply run:
\begin{lstlisting}[language=bash]
make
\end{lstlisting}

To clean previous builds, run:
\begin{lstlisting}[language=bash]
make clean
\end{lstlisting}

\section{Running the Program}
After compiling, you can run the provided utility programs that use the implemented exFAT interface. For example:
\begin{itemize}[noitemsep]
    \item \textbf{cat:} Display the contents of a file.
    \begin{lstlisting}[language=bash]
./cat Drifika.img README.md
    \end{lstlisting}
    \item \textbf{ls:} List the contents of a directory.
    \begin{lstlisting}[language=bash]
./ls Drifika.img Juventus
    \end{lstlisting}
    \item \textbf{paste:} Merge or display contents from multiple files.
    \begin{lstlisting}[language=bash]
./paste Drifika.img README.md Uefa.txt
    \end{lstlisting}
\end{itemize}

Ensure that the file system image (e.g., \texttt{Drifika.img}) and the target files or directories exist and follow the exFAT format as specified by the assignment.

\section{Implementation Details}
\subsection*{Mounting (\texttt{nqp\_mount})}
The function opens the file system image and reads the MBR. It then validates:
\begin{itemize}[noitemsep]
    \item \textbf{FileSystemName:} Must be \texttt{"EXFAT   "}.
    \item \textbf{MustBeZero:} A 53-byte region that must be all zeros.
    \item \textbf{Boot Signature:} Must be \texttt{0xAA55}.
    \item \textbf{FirstClusterOfRootDirectory:} Must be $\geq 2$.
\end{itemize}

\subsection*{Opening Files (\texttt{nqp\_open})}
The function tokenizes the provided pathname and traverses the directory structure by reading directory entries. It verifies the entry set (which comprises a file entry, a stream extension, and a file name entry) and populates an open file table.

\subsection*{Reading Files (\texttt{nqp\_read})}
Reads file data by:
\begin{itemize}[noitemsep]
    \item Calculating the appropriate cluster and offset based on the file’s current offset.
    \item Following the FAT chain to access non-contiguous clusters.
    \item Copying data into the user-supplied buffer and updating the file’s offset.
\end{itemize}

\subsection*{Directory Listing (\texttt{nqp\_getdents})}
Uses a stateful approach to iterate through a directory’s entries one at a time. This function converts the Unicode filenames to ASCII and returns an \texttt{nqp\_dirent} structure for each entry.

\subsection*{File Size (\texttt{nqp\_size})}
Scans the root directory to locate the file matching the provided file descriptor and returns its size.

\section{Testing \& Evaluation}
This implementation has been tested using various exFAT volumes provided as part of the assignment. These volumes include both simple cases (e.g., files contained in a single cluster) and more complex cases (e.g., directories or files spanning multiple clusters). The code has been designed with defensive programming in mind, checking for invalid inputs and error conditions throughout.

\section{Submission Contents}
Your submission should include at least the following files:
\begin{itemize}[noitemsep]
    \item \texttt{Makefile}
    \item \texttt{README.md}
    \item Source files (\texttt{.c} and \texttt{.h}) containing your implementation
\end{itemize}

\textbf{Note:} Do not include any disk images in your submission.

\section{Conclusion}
This assignment demonstrates a functional implementation of a read-only exFAT file system interface that supports basic file and directory operations. The project adheres to the COMP 3430 Assignment 1 specifications and includes a main testing interface to showcase the functionality of the API.

\end{document}
