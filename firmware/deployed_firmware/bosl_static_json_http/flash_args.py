Import("env")

env.Append(
    CPPDEFINES=[("BOARDNAME",  env.StringifyMacro(env.GetProjectOption("bosl_boardname"))), ("SLEEP_TIME", env.StringifyMacro(env.GetProjectOption(
        "bosl_sleeptime"))), ("SERVER", env.StringifyMacro(env.GetProjectOption("bosl_server"))), ("TOKEN", env.StringifyMacro(env.GetProjectOption("bosl_token")))]
)
