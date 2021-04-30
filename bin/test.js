var global = global || (function() { return this; }());

(function(global) {
    log("---hello---")
}(global))

function test_math()
{
    log(Math.abs(-100))
    log(Math.abs(100))
}

function test_call_c()
{
    let result = GetGameInstance(123, "jerry");
    log(result)
}

function test_parse_xml()
{
    let obj = LoadXml("Equip.xml")
    let jsonstr = JSON.stringify(obj, null, "\t")
    log(jsonstr)
}

test_math()
test_call_c()
test_parse_xml()

"C++ will get this value"
