typedef struct Builtin {
	const char *name;
	RunResult (*fn)(InterEnv *env, const char *name, size_t nargs, const Node **args);
	bool enabled;
} Builtin;

Builtin *getBuiltin(const char *name);
