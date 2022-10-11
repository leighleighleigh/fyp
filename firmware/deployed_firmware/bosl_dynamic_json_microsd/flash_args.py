Import("env")

env.Append(
    CPPDEFINES=[("BOARDNAME",  env.StringifyMacro(env.GetProjectOption("bosl_boardname")))],
)