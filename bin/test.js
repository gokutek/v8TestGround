function dump_obj(obj)
{
	for (let k in obj)
	{
		
		if (typeof(obj[k]) == "object")
		{
			dump_obj(obj[k])
		}
		else
		{
			log(k+"="+obj[k])
		}
	}
}

log("hello world")

let obj = LoadXml("Equip.xml")
//log(JSON.stringify(obj))
dump_obj(obj)

let result = GetGameInstance(123, "jerry");
'Hello, World! GetGameInstance() return ' + result;
