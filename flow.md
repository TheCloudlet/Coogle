# The Coogle Pipeline: C++ Function Signature Search with Zero-Allocation

Philosophy: Treat C++ source code as searchable data through semantic type normalization and arena-based zero-allocation design

```lisp
(defpipeline Coogle

  ;; ========================================================================
  ;; PHASE 1: INPUT & VALIDATION
  ;; ========================================================================
  (stage :input
    (receive (cli-args
               :path "src/"                    ; File or directory path
               :query "int(int, int)"))        ; Function signature query

    (validate-args
      (assert (= (length cli-args) 3))
      (assert (path-exists? :path)))

    (discover-files
      (if (file? :path)
          (list :path)
          (recursive-find :path
            :extensions [".c" ".cpp" ".cc" ".cxx"
                        ".h" ".hpp" ".hh" ".hxx"])))

    (produce
      :files ["src/foo.cpp" "src/bar.cpp" ...]
      :query-string "int(int, int)"))

  ;; ========================================================================
  ;; PHASE 2: QUERY PARSING (User's Search Pattern)
  ;; ========================================================================
  (stage :query-parser
    (input :query-string "int(int, int)")

    ;; Create arena for zero-allocation string storage
    (allocate-arena SignatureStorage
      :strings (StringArena)           ; Bump allocator for strings
      :arg-buffer (vector)             ; Temporary arg storage
      :arg-norm-buffer (vector))       ; Temporary normalized arg storage

    ;; Parse the signature string
    (call parseFunctionSignature
      (find-paren                      ; "int(int, int)"
        :open-pos 3                    ;     ^ position 3
        :close-pos 12)                 ;            ^ position 12

      ;; Extract return type
      (parse-return-type
        (trim "int")                   ; Before opening paren
        (intern-string "int")          ; Copy to arena → string_view
        (normalize-type "int"          ; Remove keywords, whitespace
          :result "int"))              ; → normalized in arena

      ;; Extract arguments
      (parse-arguments "int, int"      ; Between parens
        (split-by-comma                ; Handle nested <>() correctly
          :level-tracking true)        ; Track bracket depth

        (for-each-arg ["int" "int"]
          (intern-string arg)          ; "int" → arena → string_view
          (normalize-type arg          ; "int" → "int" (no change)
            :remove ["const" "class" "struct" "union"]
            :remove-whitespace true
            :std-string-norm true)))   ; basic_string<> → string

      (produce Signature
        :RetType "int"                 ; string_view → arena
        :RetTypeNorm "int"            ; string_view → arena
        :ArgTypes ["int", "int"]      ; span<string_view>
        :ArgTypesNorm ["int", "int"])) ; span<string_view>

    (store :target-signature))

  ;; ========================================================================
  ;; PHASE 3: LIBCLANG INITIALIZATION (The Parser Factory)
  ;; ========================================================================
  (stage :clang-setup
    (create-index CXIndex
      :raii-wrapper CXIndexRAII)       ; Auto-cleanup on destruction

    (configure-compiler-args
      ["-x" "c++"                      ; Treat as C++
       "-nostdinc"                      ; Don't parse system headers
       "-nostdinc++"])                  ; Performance optimization

    (set-parse-options
      :skip-function-bodies true       ; Only need signatures
      :incomplete true))               ; Allow incomplete parsing

  ;; ========================================================================
  ;; PHASE 4: SOURCE CODE PARSING (Per File)
  ;; ========================================================================
  (for-each-file file in :files

    (stage :libclang-frontend
      (input file "src/foo.cpp")

      ;; Parse C++ source → AST
      (call clang_parseTranslationUnit
        :index CXIndex
        :filename "src/foo.cpp"
        :compiler-args ["-x" "c++" "-nostdinc" "-nostdinc++"]
        :options [:skip-function-bodies :incomplete])

      (produce CXTranslationUnit
        :ast (syntax-tree)))           ; Full AST of the file

    ;; ======================================================================
    ;; PHASE 5: AST TRAVERSAL (The Extraction)
    ;; ======================================================================
    (stage :ast-visitor
      (create-context VisitorContext
        :target-sig (ref :target-signature)
        :current-file "src/foo.cpp"
        :results (ref :accumulator)
        :storage (ref :match-storage))  ; Shared arena for all matches

      ;; Recursive AST walk
      (call clang_visitChildren
        :root (clang_getTranslationUnitCursor TU)
        :visitor-fn visitor-callback
        :context VisitorContext)

      ;; Visitor callback (called for each AST node)
      (defn visitor-callback [cursor parent context]
        (let [kind (clang_getCursorKind cursor)]

          ;; Filter: only process function declarations
          (when (or (= kind CXCursor_FunctionDecl)
                   (= kind CXCursor_CXXMethod))

            ;; ==============================================================
            ;; PHASE 6: TYPE EXTRACTION & CANONICALIZATION (The Key!)
            ;; ==============================================================
            (stage :type-canonicalization

              ;; Get return type (CANONICAL TYPE RESOLUTION HERE!)
              (extract-return-type
                (clang_getCursorResultType cursor)    ; CXType
                (clang_getCanonicalType RetType)      ; ← TYPE ALIAS RESOLUTION!
                (clang_getTypeSpelling CanonicalType) ; "unsigned long long"
                                                      ; NOT "uint64_t"
                (intern-string RetTypeSV)             ; → arena
                (normalize-type RetTypeInterned       ; Remove const, etc.
                  :result RetTypeNorm))

              ;; Get arguments (ALSO CANONICALIZED!)
              (for-each-arg-index idx in [0..NumArgs]
                (get-arg
                  (clang_Cursor_getArgument cursor idx)    ; CXCursor
                  (clang_getCursorType ArgCursor)          ; CXType
                  (clang_getCanonicalType ArgType)         ; ← CANONICALIZE!
                  (clang_getTypeSpelling CanonicalArgType) ; "int" not "MyInt"
                  (intern-string ArgTypeSV)                ; → arena
                  (normalize-type ArgTypeInterned          ; Remove const, etc.
                    :result ArgTypeNorm)))

              ;; Build signature from libclang data
              (construct Signature
                :RetType RetTypeInterned      ; From libclang (canonical)
                :RetTypeNorm RetTypeNorm      ; Further normalized
                :ArgTypes (span args)         ; From libclang (canonical)
                :ArgTypesNorm (span argsNorm))) ; Further normalized

            ;; ==============================================================
            ;; PHASE 7: SIGNATURE MATCHING (Semantic Comparison)
            ;; ==============================================================
            (stage :matcher
              (input
                :user-signature (from :query-parser)
                :actual-signature (from :type-canonicalization))

              (call isSignatureMatch

                ;; Compare return types (O(1) string_view comparison)
                (compare-return-types
                  user-sig.RetTypeNorm        ; "int"
                  actual-sig.RetTypeNorm      ; "int"
                  (= ...))                    ; true/false

                ;; Compare argument count
                (compare-arg-count
                  (length user-sig.ArgTypesNorm)
                  (length actual-sig.ArgTypesNorm)
                  (= ...))

                ;; Compare each argument (with wildcard support)
                (for-each-arg-pair user-arg actual-arg
                  (if (= user-arg "*")        ; Wildcard matches anything
                      :match
                      (= user-arg.norm        ; O(1) comparison
                         actual-arg.norm))))  ; Pre-normalized!

              ;; If match found...
              (when :match

                ;; ========================================================
                ;; PHASE 8: RESULT EXTRACTION
                ;; ========================================================
                (stage :result-capture
                  (extract-location
                    (clang_getCursorLocation cursor)
                    (clang_getSpellingLocation Location
                      :file File
                      :line 42
                      :column nil))

                  (extract-name
                    (clang_getCursorSpelling cursor)
                    :name "addNumbers")

                  ;; Only include if from current file (not #included)
                  (when (= filename context.current-file)

                    ;; Intern strings into SHARED arena (zero-copy)
                    (record-match
                      :function-name (intern "addNumbers")
                      :signature-str (toString actual-signature)
                      :line-number 42)

                    ;; Accumulate in results vector
                    (push context.results
                      (Match
                        :FileName "src/foo.cpp"  ; string_view
                        :Matches [{:name "addNumbers"
                                  :sig "int(int, int)"
                                  :line 42}])))))))

          ;; Continue traversing AST
          CXChildVisit_Recurse))))

  ;; ========================================================================
  ;; PHASE 9: NORMALIZATION DETAILS (The Semantic Engine)
  ;; ========================================================================
  (stage :type-normalizer
    (defn normalizeType [arena type-string]
      "Two-pass normalization for semantic type matching"

      ;; Allocate arena buffer (no malloc!)
      (allocate-buffer arena (* 2 (length type-string)))

      ;; PASS 1: Remove noise (syntax → semantics)
      (for-each-char c in type-string
        (cond
          ;; Skip whitespace
          ((whitespace? c) :skip)

          ;; Skip qualifiers (const, class, struct, union)
          ;; These are semantic noise for function matching
          ((keyword? c :at-word-boundary)
           (skip-keyword ["const" "class" "struct" "union"]))

          ;; Keep everything else
          (else (write-buffer c))))

      ;; PASS 2: Normalize template aliases
      ;; "std::basic_string<char, ...>" → "std::string"
      (normalize-std-string buffer
        (find "std::basic_string")
        (match-brackets)               ; Handle nested <>
        (replace-with "std::string"))

      ;; Finalize: return string_view into arena (zero-copy!)
      (finalize arena buffer)))

  ;; ========================================================================
  ;; PHASE 10: OUTPUT FORMATTING
  ;; ========================================================================
  (stage :output
    (print-header
      (format "▶ Searching for: {}"
        (toString target-signature)))  ; "int(int, int)"

    (for-each-file-result result in :results
      (print-file
        (format "✔ {}" result.FileName))

      (for-each-match match in result.Matches
        (print-match
          (format "  └─ {}: {}"
            match.Line
            match.FunctionName
            match.SignatureStr)))      ; "int(int, int)"

      (increment :total-matches))

    (print-summary
      (format "\nMatches found: {}" :total-matches)))

  ;; ========================================================================
  ;; KEY ARCHITECTURAL INSIGHTS
  ;; ========================================================================
  (insights

    ;; 1. Zero-Allocation Design
    ;;    - Arena allocator (bump pointer) for all strings
    ;;    - string_view references instead of copies
    ;;    - No malloc during hot path (only arena bump)

    ;; 2. Semantic Type Matching
    ;;    - clang_getCanonicalType() resolves ALL type aliases
    ;;    - typedef/using declarations become underlying types
    ;;    - Searches are semantically correct, not just textual

    ;; 3. Pre-Normalization Cache
    ;;    - Each Signature stores BOTH original and normalized types
    ;;    - Matching is O(1) string_view comparison
    ;;    - No repeated normalization during matching

    ;; 4. Two-Level Normalization
    ;;    Level 1 (libclang): Canonical type resolution
    ;;      - typedef int MyInt → int
    ;;      - using Ptr = int* → int*
    ;;      - std::string → std::basic_string<char, ...>
    ;;    Level 2 (our parser): Syntactic normalization
    ;;      - Remove: const, class, struct, union
    ;;      - Remove: all whitespace
    ;;      - Normalize: std::basic_string<...> → std::string

    ;; 5. Wildcard Support
    ;;    - "*" in user query matches any type
    ;;    - "int(*, *)" matches "int(char, double)"
    ;;    - Implemented by checking original (non-normalized) args

    ;; 6. Data Flow Pattern
    ;;    Source .cpp → libclang → Canonical Types → Normalize →
    ;;    Store (arena) → Compare (O(1)) → Match → Output))

;; ============================================================================
;; END PIPELINE
;; ============================================================================
```

## Visual Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         COOGLE ARCHITECTURE                              │
└─────────────────────────────────────────────────────────────────────────┘

INPUT PHASE
───────────
    CLI Args: "src/" + "int(int, int)"
           │
           ├─→ File Discovery → [foo.cpp, bar.cpp, ...]
           │
           └─→ Query Parser
                    │
                    ▼
              ┌─────────────┐
              │ StringArena │ (Bump allocator)
              └─────────────┘
                    │
                    ├─→ parseFunctionSignature("int(int, int)")
                    │        │
                    │        ├─→ Parse: "int" + "(int, int)"
                    │        ├─→ Intern strings → arena
                    │        └─→ Normalize → "int" (no change)
                    │
                    ▼
              ┌─────────────────────────────┐
              │ Target Signature            │
              │  RetType:     "int"         │
              │  RetTypeNorm: "int"         │
              │  ArgTypes:    ["int","int"] │
              │  ArgTypesNorm:["int","int"] │
              └─────────────────────────────┘

PARSING PHASE (per file)
─────────────
    Source File: "src/foo.cpp"
           │
           ▼
    ┌──────────────────┐
    │   libclang       │
    │ (LLVM Frontend)  │
    └──────────────────┘
           │
           ├─→ clang_parseTranslationUnit()
           │        ├─→ Lexer: tokens
           │        ├─→ Parser: syntax tree
           │        └─→ Sema: type checking
           │
           ▼
    ┌──────────────────┐
    │  AST (Abstract   │
    │  Syntax Tree)    │
    └──────────────────┘
           │
           ▼
    clang_visitChildren() ──→ Recursive traversal
           │
           └─→ For each node:
                    │
                    ▼
              Is FunctionDecl?
                    │
                    ├─ NO → Continue
                    │
                    └─ YES
                         │
                         ▼

TYPE EXTRACTION & CANONICALIZATION (THE CRITICAL STEP!)
───────────────────────────────────
    Function Declaration AST Node
           │
           ├─→ clang_getCursorResultType()
           │        │
           │        ▼
           │   CXType (raw type)
           │        │
           │        ▼
           │   clang_getCanonicalType()  ← TYPE ALIAS RESOLUTION!
           │        │
           │        │   Example transformations:
           │        │   - typedef int MyInt       → int
           │        │   - using uint64 = unsigned long long → unsigned long long
           │        │   - std::string             → std::basic_string<char,...>
           │        │
           │        ▼
           │   CXType (canonical)
           │        │
           │        ▼
           │   clang_getTypeSpelling() → "unsigned long long"
           │        │
           │        └─→ Intern to arena
           │                 │
           │                 ▼
           │            normalizeType()
           │                 │
           │                 ├─→ Remove: const, class, struct, union
           │                 ├─→ Remove: whitespace
           │                 └─→ Normalize: std::basic_string → std::string
           │                      │
           │                      ▼
           │                 "unsignedlonglong" (normalized)
           │
           └─→ Same process for each argument type
                    │
                    ▼

    ┌─────────────────────────────────────┐
    │ Actual Signature (from source)      │
    │  RetType:     "unsigned long long"  │
    │  RetTypeNorm: "unsignedlonglong"    │
    │  ArgTypes:    [...]                 │
    │  ArgTypesNorm:[...]                 │
    └─────────────────────────────────────┘

MATCHING PHASE
──────────────
    ┌─────────────────┐      ┌─────────────────┐
    │ Target Sig      │      │ Actual Sig      │
    │ (from query)    │      │ (from source)   │
    └─────────────────┘      └─────────────────┘
           │                          │
           └──────────┬───────────────┘
                      │
                      ▼
              isSignatureMatch()
                      │
                      ├─→ Compare RetTypeNorm (O(1) string_view)
                      ├─→ Compare arg count
                      └─→ For each arg:
                               ├─ Is wildcard "*"? → MATCH
                               └─ Compare ArgTypeNorm (O(1))
                      │
                      ▼
                  MATCH / NO MATCH
                      │
                      └─ IF MATCH
                              │
                              ▼

RESULT CAPTURE
──────────────
    Extract metadata:
           │
           ├─→ Function name: "addNumbers"
           ├─→ Source location: line 42
           └─→ Signature string: "int(int, int)"
                    │
                    ▼
              Intern to SHARED arena (zero-copy!)
                    │
                    ▼
              Accumulate in results vector

OUTPUT PHASE
────────────
    Format and display:

    ▶ Searching for: int(int, int)

    ✔ src/foo.cpp
      └─ 42: addNumbers int(int, int)

    Matches found: 1

MEMORY LAYOUT (Arena Design)
─────────────────────────────

    StringArena (bump allocator)
    ┌───────────────────────────────────────┐
    │ "int" │ "unsigned long long" │ "void" │ ...
    └───────────────────────────────────────┘
       ▲           ▲                   ▲
       │           │                   │
    string_view string_view       string_view
    (ptr + len)  (ptr + len)      (ptr + len)

    - No malloc() during hot path
    - All strings contiguous in memory
    - string_view: zero-copy references
    - Entire arena freed at once

PERFORMANCE CHARACTERISTICS
────────────────────────────
    - String comparison:  O(1) via string_view
    - Type normalization: O(n) per type, cached
    - Signature matching: O(args) with early exit
    - Memory allocation:  O(1) bump pointer
    - File parsing:       O(AST nodes) linear scan
```

## Code Mapping

| Phase | Function/File | Line Numbers |
|-------|---------------|--------------|
| Input & Validation | `main.cpp:main()` | 177-213 |
| Query Parsing | `parser.cpp:parseFunctionSignature()` | 146-241 |
| Libclang Setup | `main.cpp:main()` | 223-241 |
| Source Parsing | `main.cpp:main()` | 249-268 |
| AST Traversal | `main.cpp:visitor()` | 79-175 |
| Type Canonicalization | `main.cpp:visitor()` | 88-121 |
| Signature Matching | `parser.cpp:isSignatureMatch()` | 248-271 |
| Result Capture | `main.cpp:visitor()` | 131-171 |
| Normalization | `parser.cpp:normalizeType()` | 118-144 |
| Output | `main.cpp:main()` | 270-291 |

## Key Observations

1. **Type aliases are resolved by libclang**, not by our parser
2. **Semantic matching** is achieved through canonical type resolution
3. **Zero-allocation** is achieved through arena allocators and string_view
4. **Performance** comes from pre-normalization and O(1) comparisons
5. **Correctness** comes from using LLVM's type system, not string matching
