const { execSync } = require("child_process");

if(process.argv.length<4) return;

const colors = JSON.parse(require("fs").readFileSync("./colors.json"));
const command = process.argv[2];
let arr = command.split(",");
let result = [];

const isNumber = (v) => {
	if(Array.isArray(v)) {
		let b = true;
		for(var n=0; n<v.length; n++) {
			b&=((typeof(v[n])==="number"||typeof(v[n])==="string")&&(""+v[n]).length&&!isNaN(+v[n]));
		}
		return b;
	}
	else {
		return ((typeof(v)==="number"||typeof(v)==="string")&&(""+v).length&&!isNaN(+v));
	}
}

const random = (min,max) => {
	const diff = max-min;
	return min + ((Math.random()*(diff+1))>>0);
}

const count = (str,re) => {
	return (str.match(re)||[]).length 
}

for(var n=0; n<arr.length; n++) {
	let err = true;
	let c = arr[n];
	while(c.length) {
		if(c[0]==="{"&&c[c.length-1]==="}") {
			c = c.slice(1,c.length-1);
		}
		else {
			break;
		}
	}
	
	if(c.indexOf(".mov")>0||c.indexOf(".mp4")>0) {
		result.push(c);
		err = false;
	}
	else {
		if(c.indexOf("rand{")===0&&c.indexOf("}")>=6) {
			if(count(c,/\{/g)===1&&count(c,/\}/g)===1) {
				let repeat = 1;
				c = c.slice("rand{".length,c.length);
				if(c[c.indexOf("}")+1]==="x") {
					const t = c.split("}x")[1];
					repeat = (isNumber(t))?+t:0;
				}
				if(repeat>=1) {
					let rand = [-1,-1];
					c = c.split("}")[0];
					if(isNumber(c)) {
						rand = [0,+c];
					}
					else if(c.indexOf("-")>=1) {
						const range = c.split("-");
						if(range.length===2&&isNumber(range[0])&&isNumber(range[1])) {
							rand = (+range[0]<+range[1])?[+range[0],+range[1]]:[+range[1],+range[0]];
						}
					}
					if(rand[0]>=0&&rand[1]>=0) {
						while(repeat--) result.push(random(rand[0],rand[1]));
						err = false;
					}
				}
			}
		}
		else if(count(c,/}\+/g)===1) {
			let tmp = c.split("}+");
			if(tmp[0][0]==="{"&&tmp[1][tmp[1].length-1]==="}") {
				tmp[0] = tmp[0].slice(1,tmp[0].length);
				tmp[1] = tmp[1].slice(0,tmp[1].length-1);
			}
			if(isNumber(tmp[1])) {
				const plus = +tmp[1];
				const num = count(tmp[0],/\{/g);
				if(num==1) {
					if(tmp[0][0]==="{") {
						c = tmp[0].slice(1,c.length);
						const range = c.split("-");
						if(range.length===2&&isNumber(range[0])&&isNumber(range[1])) {
							result.push(((+range[0])+plus)+"-"+((+range[1])+plus));
							err = false;
						}
					}
					else if(count(tmp[0],/\x{/g)===1) {
						tmp = tmp[0].split("x{");
						if(isNumber(tmp[0])) {
							const scale = tmp[0];
							const range = tmp[1].split("-");
							if(range.length===2&&isNumber(range[0])&&isNumber(range[1])) {
								const begin = +range[0];
								const end = +range[1];
								if(begin<end) {
									for(var k=begin; k<=end; k++) {
										result.push((scale*k)+plus);
									}
								}
								else {
									for(var k=begin; k>=end; k--) {
										result.push((scale*k)+plus);
									}
								}
								err = false;
							}
						}
					}
				}
			}
			else {
				if(isNumber(tmp[1].split("}x"))) {
					const plus = +tmp[1].split("}x")[0];
					let repeat = +tmp[1].split("}x")[1];
					if(tmp[0].indexOf("{{")===0) {
						const range = tmp[0].slice(2,c.length).split("-");
						if(range.length===2&&isNumber(range[0])&&isNumber(range[1])) {
							while(repeat--) {
								const begin = +range[0];
								const end = +range[1];
								result.push((begin+plus)+"-"+(end+plus));
							}
							err = false;
						}
					}
					else if(tmp[0].indexOf("{")===0&&tmp[0].indexOf("x{")>=2) {
						tmp[0] = tmp[0].slice(1,tmp[0].length);
						if(isNumber(tmp[0].split("x{")[0])) {
							const scale = +tmp[0].split("x{")[0];
							const range = tmp[0].split("x{")[1].split("-");
							if(range.length===2&&isNumber(range[0])&&isNumber(range[1])) {
								while(repeat--) {
									const begin = +range[0];
									const end = +range[1];
									if(begin<end) {
										for(var k=begin; k<=end; k++) {
											result.push((scale*k)+plus);
										}
									}
									else {
										for(var k=begin; k>=end; k--) {
											result.push((scale*k)+plus);
										}
									}
								}
								err = false;
							}
						}
					}
				}
			}
		}
		else if(c.indexOf("x{")>=1&&c[c.length-1]=="}") {
			c = c.slice(0,c.length-1);
			if(isNumber(c.split("x{")[0])) {
				const scale = +c.split("x{")[0];
				c = c.split("x{")[1];
				if(isNumber(c)) {
					while(repeat--) result.push((+c)*scale);
					err = false;
				}
				else if(c.indexOf("-")>=1) {
					const range = c.split("-");
					if(range.length===2&&isNumber(range[0])&&isNumber(range[1])) {
						const begin = +range[0];
						const end = +range[1];
						const step = (begin<end)?1:-1;
						if(begin<end) {
							for(var k=begin; k<=end; k++) {
								result.push(scale*k);
							}
						}
						else {
							for(var k=begin; k>=end; k--) {
								result.push(scale*k);
							}
						}
						err = false;
					}
				}
			}
		}
		else if(c.indexOf("{")===0&&c.indexOf("}x")>=2) {
			let repeat = 1;
			c = c.slice(1,c.length);
			const t = c.split("}x")[1];
			repeat = (isNumber(t))?+t:0;
			if(repeat>0) {
				c = c.split("}x")[0];
				while(c.length) {
					if(c[0]==="{"&&c[c.length-1]==="}") {
						c = c.slice(1,c.length-1);
					}
					else {
						break;
					}
				}
				if(isNumber(c)) {
					while(repeat--) result.push(c);
					err = false;
				}
				else if(c.indexOf("-")>=1) {
					const range = c.split("-");
					if(range.length===2&&isNumber(range[0])&&isNumber(range[1])) {
						if(range[0]===range[1]) c = range[0];
						while(repeat--) result.push(c);
						err = false;
					}
				}
			}
		}
		else if(c.indexOf("-")>=1) {
			const range = c.split("-");
			if(range.length===2&&isNumber(range[0])&&isNumber(range[1])) {
				result.push((range[0]===range[1])?range[0]:c);
				err = false;
			}
		}
		else if(c.indexOf("x")>=1) {
			const range = c.split("x");
			if(range.length===2&&isNumber(range[0])&&isNumber(range[1])) {
				const repeat = +range[1];
				if(repeat>=1) {
					result.push((repeat===1)?range[0]:c);
					err = false;
				}
			}
		}
		else if(isNumber(+c)) {
			result.push(c);
			err = false;
		}
		else if(colors[c]) {
			result.push(c);
			err = false;
		}
	}
	if(err) console.log("? "+arr[n])
}
if(result.length>=1) execSync(`
cd ${__dirname}
set -eu
./VideoEditor ${result.join(",")} ${process.argv[3]}
open ${process.argv[3]}
`);