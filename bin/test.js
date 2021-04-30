let obj = LoadXml("Equip.xml")
let jsonstr = JSON.stringify(obj, null, "\t")
log(jsonstr)

let result = GetGameInstance(123, "jerry");
'Hello, World! GetGameInstance() return ' + result;
