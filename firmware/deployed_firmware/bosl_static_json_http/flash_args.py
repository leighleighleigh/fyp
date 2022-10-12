Import("env")

env.Append(
    CPPDEFINES=[("CELLNUMBER",  env.StringifyMacro(env.GetProjectOption("bosl_cellnumber"))),("BOARDNAME",  env.StringifyMacro(env.GetProjectOption("bosl_boardname"))), ("SLEEP_TIME", env.GetProjectOption("bosl_sleeptime"))]
)
