/* crypt.c - code for en/decrypting files
 *
 * Copyright (C) 2011 - Brian Johnson <brijohn@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *
 */

#include <string.h>

static unsigned long table1[] = {
	0x3F7EB852, // dword_3A8160
	0x3F81930C, // dword_3A8164
	0x3F86F34D, // dword_3A8168
	0x3FC1930C, // dword_3A816C
	0x40161E4F, // dword_3A8170
	0x3F004EA5, // dword_3A8174
	0x3F885F07, // dword_3A8178
	0x3B83126F, // dword_3A817C
	0x3243f6c,  // dword_3a8180
	0x1921fb6,  // dword_3a8184
	0x3fc90fdb, // dword_3a8188
};

static unsigned long table2[] = {
	0x31A4B7A0, // dword_3A81A0
	0x3BFFFF55, // dword_3A81A4
	0x3C7FFD55, // dword_3A81A8
	0x3CBFFB80, // dword_3A81AC
	0x3CFFF555, // dword_3A81B0
	0x3D1FF596, // dword_3A81B4
	0x3D3FEE01, // dword_3A81B8
	0x3D5FE36C, // dword_3A81BC
	0x3D7FD557, // dword_3A81C0
	0x3D8FE1A2, // dword_3A81C4
	0x3D9FD659, // dword_3A81C8
	0x3DAFC890, // dword_3A81CC
	0x3DBFB808, // dword_3A81D0
	0x3DCFA481, // dword_3A81D4
	0x3DDF8DBC, // dword_3A81D8
	0x3DEF7379, // dword_3A81DC
	0x3DFF5577, // dword_3A81E0
	0x3E0799BC, // dword_3A81E4
	0x3E0F869F, // dword_3A81E8
	0x3E177143, // dword_3A81EC
	0x3E1F5989, // dword_3A81F0
	0x3E273F52, // dword_3A81F4
	0x3E2F227E, // dword_3A81F8
	0x3E3702EE, // dword_3A81FC
	0x3E3EE081, // dword_3A8200
	0x3E46BB19, // dword_3A8204
	0x3E4E9297, // dword_3A8208
	0x3E5666D9, // dword_3A820C
	0x3E5E37C2, // dword_3A8210
	0x3E660533, // dword_3A8214
	0x3E6DCF0B, // dword_3A8218
	0x3E75952C, // dword_3A821C
	0x3E7D5777, // dword_3A8220
	0x3E828AE6, // dword_3A8224
	0x3E866806, // dword_3A8228
	0x3E8A430D, // dword_3A822C
	0x3E8E1BEB, // dword_3A8230
	0x3E91F291, // dword_3A8234
	0x3E95C6EE, // dword_3A8238
	0x3E9998F5, // dword_3A823C
	0x3E9D6895, // dword_3A8240
	0x3EA135BF, // dword_3A8244
	0x3EA50065, // dword_3A8248
	0x3EA8C876, // dword_3A824C
	0x3EAC8DE5, // dword_3A8250
	0x3EB050A1, // dword_3A8254
	0x3EB4109C, // dword_3A8258
	0x3EB7CDC7, // dword_3A825C
	0x3EBB8813, // dword_3A8260
	0x3EBF3F70, // dword_3A8264
	0x3EC2F3D1, // dword_3A8268
	0x3EC6A525, // dword_3A826C
	0x3ECA535F, // dword_3A8270
	0x3ECDFE70, // dword_3A8274
	0x3ED1A649, // dword_3A8278
	0x3ED54ADB, // dword_3A827C
	0x3ED8EC18, // dword_3A8280
	0x3EDC89F2, // dword_3A8284
	0x3EE02459, // dword_3A8288
	0x3EE3BB3F, // dword_3A828C
	0x3EE74E97, // dword_3A8290
	0x3EEADE52, // dword_3A8294
	0x3EEE6A61, // dword_3A8298
	0x3EF1F2B6, // dword_3A829C
	0x3EF57744, // dword_3A82A0
	0x3EF8F7FB, // dword_3A82A4
	0x3EFC74CF, // dword_3A82A8
	0x3EFFEDB1, // dword_3A82AC
	0x3F01B14A, // dword_3A82B0
	0x3F0369B4, // dword_3A82B4
	0x3F052011, // dword_3A82B8
	0x3F06D459, // dword_3A82BC
	0x3F088686, // dword_3A82C0
	0x3F0A3691, // dword_3A82C4
	0x3F0BE473, // dword_3A82C8
	0x3F0D9025, // dword_3A82CC
	0x3F0F39A2, // dword_3A82D0
	0x3F10E0E1, // dword_3A82D4
	0x3F1285DD, // dword_3A82D8
	0x3F14288E, // dword_3A82DC
	0x3F15C8EF, // dword_3A82E0
	0x3F1766F9, // dword_3A82E4
	0x3F1902A6, // dword_3A82E8
	0x3F1A9BEE, // dword_3A82EC
	0x3F1C32CC, // dword_3A82F0
	0x3F1DC739, // dword_3A82F4
	0x3F1F592F, // dword_3A82F8
	0x3F20E8A7, // dword_3A82FC
	0x3F22759C, // dword_3A8300
	0x3F240007, // dword_3A8304
	0x3F2587E2, // dword_3A8308
	0x3F270D27, // dword_3A830C
	0x3F288FD0, // dword_3A8310
	0x3F2A0FD6, // dword_3A8314
	0x3F2B8D35, // dword_3A8318
	0x3F2D07E5, // dword_3A831C
	0x3F2E7FE1, // dword_3A8320
	0x3F2FF523, // dword_3A8324
	0x3F3167A5, // dword_3A8328
	0x3F32D761, // dword_3A832C
	0x3F344452, // dword_3A8330
	0x3F35AE73, // dword_3A8334
	0x3F3715BC, // dword_3A8338
	0x3F387A29, // dword_3A833C
	0x3F39DBB4, // dword_3A8340
	0x3F3B3A58, // dword_3A8344
	0x3F3C960F, // dword_3A8348
	0x3F3DEED3, // dword_3A834C
	0x3F3F44A0, // dword_3A8350
	0x3F40976F, // dword_3A8354
	0x3F41E73D, // dword_3A8358
	0x3F433402, // dword_3A835C
	0x3F447DBB, // dword_3A8360
	0x3F45C462, // dword_3A8364
	0x3F4707F2, // dword_3A8368
	0x3F484866, // dword_3A836C
	0x3F4985B8, // dword_3A8370
	0x3F4ABFE5, // dword_3A8374
	0x3F4BF6E6, // dword_3A8378
	0x3F4D2AB8, // dword_3A837C
	0x3F4E5B55, // dword_3A8380
	0x3F4F88B8, // dword_3A8384
	0x3F50B2DE, // dword_3A8388
	0x3F51D9C1, // dword_3A838C
	0x3F52FD5C, // dword_3A8390
	0x3F541DAB, // dword_3A8394
	0x3F553AAA, // dword_3A8398
	0x3F565454, // dword_3A839C
	0x3F576AA4, // dword_3A83A0
	0x3F587D97, // dword_3A83A4
	0x3F598D28, // dword_3A83A8
	0x3F5A9953, // dword_3A83AC
	0x3F5BA214, // dword_3A83B0
	0x3F5CA765, // dword_3A83B4
	0x3F5DA945, // dword_3A83B8
	0x3F5EA7AD, // dword_3A83BC
	0x3F5FA29B, // dword_3A83C0
	0x3F609A0B, // dword_3A83C4
	0x3F618DF8, // dword_3A83C8
	0x3F627E5F, // dword_3A83CC
	0x3F636B3C, // dword_3A83D0
	0x3F64548B, // dword_3A83D4
	0x3F653A49, // dword_3A83D8
	0x3F661C72, // dword_3A83DC
	0x3F66FB02, // dword_3A83E0
	0x3F67D5F7, // dword_3A83E4
	0x3F68AD4C, // dword_3A83E8
	0x3F6980FF, // dword_3A83EC
	0x3F6A510B, // dword_3A83F0
	0x3F6B1D6F, // dword_3A83F4
	0x3F6BE626, // dword_3A83F8
	0x3F6CAB2D, // dword_3A83FC
	0x3F6D6C81, // dword_3A8400
	0x3F6E2A20, // dword_3A8404
	0x3F6EE407, // dword_3A8408
	0x3F6F9A31, // dword_3A840C
	0x3F704C9E, // dword_3A8410
	0x3F70FB49, // dword_3A8414
	0x3F71A630, // dword_3A8418
	0x3F724D51, // dword_3A841C
	0x3F72F0A8, // dword_3A8420
	0x3F739034, // dword_3A8424
	0x3F742BF1, // dword_3A8428
	0x3F74C3DE, // dword_3A842C
	0x3F7557F7, // dword_3A8430
	0x3F75E83C, // dword_3A8434
	0x3F7674A8, // dword_3A8438
	0x3F76FD3B, // dword_3A843C
	0x3F7781F2, // dword_3A8440
	0x3F7802CB, // dword_3A8444
	0x3F787FC4, // dword_3A8448
	0x3F78F8DA, // dword_3A844C
	0x3F796E0D, // dword_3A8450
	0x3F79DF5B, // dword_3A8454
	0x3F7A4CC0, // dword_3A8458
	0x3F7AB63D, // dword_3A845C
	0x3F7B1BCF, // dword_3A8460
	0x3F7B7D74, // dword_3A8464
	0x3F7BDB2B, // dword_3A8468
	0x3F7C34F3, // dword_3A846C
	0x3F7C8ACA, // dword_3A8470
	0x3F7CDCAF, // dword_3A8474
	0x3F7D2AA1, // dword_3A8478
	0x3F7D749E, // dword_3A847C
	0x3F7DBAA5, // dword_3A8480
	0x3F7DFCB5, // dword_3A8484
	0x3F7E3ACD, // dword_3A8488
	0x3F7E74EC, // dword_3A848C
	0x3F7EAB11, // dword_3A8490
	0x3F7EDD3C, // dword_3A8494
	0x3F7F0B6B, // dword_3A8498
	0x3F7F359F, // dword_3A849C
	0x3F7F5BD5, // dword_3A84A0
	0x3F7F7E0E, // dword_3A84A4
	0x3F7F9C49, // dword_3A84A8
	0x3F7FB685, // dword_3A84AC
	0x3F7FCCC3, // dword_3A84B0
	0x3F7FDF01, // dword_3A84B4
	0x3F7FED40, // dword_3A84B8
	0x3F7FF77F, // dword_3A84BC
	0x3F7FFDBF, // dword_3A84C0
	0x3F7FFFFE, // dword_3A84C4
	0x3F7FFE3D, // dword_3A84C8
	0x3F7FF87D, // dword_3A84CC
	0x3F7FEEBC, // dword_3A84D0
	0x3F7FE0FC, // dword_3A84D4
	0x3F7FCF3D, // dword_3A84D8
	0x3F7FB97E, // dword_3A84DC
	0x3F7F9FC0, // dword_3A84E0
	0x3F7F8204, // dword_3A84E4
	0x3F7F6049, // dword_3A84E8
	0x3F7F3A92, // dword_3A84EC
	0x3F7F10DD, // dword_3A84F0
	0x3F7EE32C, // dword_3A84F4
	0x3F7EB17F, // dword_3A84F8
	0x3F7E7BD8, // dword_3A84FC
	0x3F7E4237, // dword_3A8500
	0x3F7E049D, // dword_3A8504
	0x3F7DC30A, // dword_3A8508
	0x3F7D7D81, // dword_3A850C
	0x3F7D3402, // dword_3A8510
	0x3F7CE68E, // dword_3A8514
	0x3F7C9526, // dword_3A8518
	0x3F7C3FCC, // dword_3A851C
	0x3F7BE681, // dword_3A8520
	0x3F7B8946, // dword_3A8524
	0x3F7B281D, // dword_3A8528
	0x3F7AC308, // dword_3A852C
	0x3F7A5A08, // dword_3A8530
	0x3F79ED1E, // dword_3A8534
	0x3F797C4C, // dword_3A8538
	0x3F790795, // dword_3A853C
	0x3F788EF9, // dword_3A8540
	0x3F78127B, // dword_3A8544
	0x3F77921D, // dword_3A8548
	0x3F770DE1, // dword_3A854C
	0x3F7685C8, // dword_3A8550
	0x3F75F9D6, // dword_3A8554
	0x3F756A0B, // dword_3A8558
	0x3F74D66B, // dword_3A855C
	0x3F743EF7, // dword_3A8560
	0x3F73A3B3, // dword_3A8564
	0x3F7304A0, // dword_3A8568
	0x3F7261C0, // dword_3A856C
	0x3F71BB18, // dword_3A8570
	0x3F7110A8, // dword_3A8574
	0x3F706274, // dword_3A8578
	0x3F6FB07F, // dword_3A857C
	0x3F6EFACB, // dword_3A8580
	0x3F6E415B, // dword_3A8584
	0x3F6D8432, // dword_3A8588
	0x3F6CC352, // dword_3A858C
	0x3F6BFEC0, // dword_3A8590
	0x3F6B367E, // dword_3A8594
	0x3F6A6A8F, // dword_3A8598
	0x3F699AF6, // dword_3A859C
	0x3F68C7B7, // dword_3A85A0
};

unsigned long sub_3a65dc(unsigned long arg_0, unsigned long arg_4)
{
	unsigned short var_c, var_18;
	unsigned short var_14, var_1c;
	unsigned short var_8;
	unsigned short var_24;
	unsigned long var_20;
	unsigned long var_10;
	unsigned long var_4;
	var_c = (arg_0 >> 16) & 0x7f80;
	var_18 = (arg_4 >> 16) & 0x7f80;
	var_14 = (arg_0 >> 16) & 0x8000;
	var_1c = (arg_4 >> 16) & 0x8000;
	var_24 = var_14 ^ var_1c;
	if (var_c == 0x7f80) {
		if (var_18 == 0x7f80)
			var_20 = 0x7fffffff;
		else
			var_20 = arg_0;
		return (var_20 | (var_24 << 16));
	}
	if (var_c == 0 || var_18 == 0) {
		var_20 = var_24 << 16;
		return var_20;
	}
	var_10 = arg_4;
	if (var_18 == 0x7f80) {
		return (var_10 | (var_24 << 16));
	}
	var_c >>= 7;
	var_18 >>= 7;
	var_4 = arg_0 & 0x7fffff;
	var_4 = var_4 | 0x800000;
	var_10 = arg_4 & 0x7fffff;
	var_10 = var_10 | 0x800000;
	var_8 = var_c + var_18 - 0x84;
	var_4 <<= 7;
	var_10 <<= 7;
	var_20 = ((long)((var_4 >> 16) & 0xffff)) * ((long)(var_10 & 0xffff));
	var_20 += ((long)(var_4 & 0xffff)) * ((long)((var_10 >> 16) & 0xffff));
	var_20 >>= 16;
	var_20 += ((long)((var_4 >> 16) & 0xffff)) * ((long)((var_10 >> 16) & 0xffff));
	if ((var_20 >> 16) & 0xff80) {
		while((var_20 >> 16) & 0xff00) {
			var_8 += 1;
			var_20 >>= 1;
		}
	} else {
		while(!((var_20 >> 16) & 0x80)) {
			var_8 -= 1;
			var_20 <<= 1;
		}
	}
	if (var_8 != 0 && (var_8 & 0x8000) == 0) {
		if ((short)var_8 < 0xff) {
			var_20 = var_20 & 0x7fffff;
			var_20 |= ((var_24 | (var_8 << 7)) << 16);
			return var_20;
		}
		var_20 = 0x7fffffff;
		var_20 |= (var_24 << 16);
		return var_20;
	} else {
		return (var_24 << 16);
	}
}

unsigned long sub_3a6b8f(unsigned long arg_0, unsigned long arg_4)
{
	unsigned long var_8;
	short var_4 = 0x96;
	if (arg_0 == 0)
		return 0;
	while ((arg_0 & 0xFF000000)) {
		arg_0 >>= 1;
		var_4 += 1;
	}
	while (!(arg_0 & 0x800000)) {
		arg_0 <<= 1;
		var_4 -= 1;
	}
	var_4 -= arg_4;
	if (var_4 <= 0)
		return 0;
	if (var_4 >= 0xff)
		return 0x7fffffff;
	var_8 = arg_0 & 0x7fffff;
	return ((var_4 << 23) | var_8);
}

unsigned long sub_3a6886(unsigned long arg_0)
{
	unsigned short var_28, var_4, var_c;
	unsigned long var_18;
	unsigned long var_10;
	unsigned long var_8;
	unsigned long var_14, var_24, var_1c, var_20;
	var_28 = (arg_0 >> 16) & 0x8000;
	var_4 = (arg_0 >> 16) & 0x7f80;
	if (var_4 == 0x7f80) {
		var_18 = 0x7fffffff | (var_28 << 16);
		return var_18;
	}
	if ((arg_0 & 0x7fffffff) > 0x43fe7811) {
		var_18 = 0;
		return var_18;
	}
	if (var_4 == 0) {
		var_18 = (var_28 << 16);
		return var_18;
	}
	var_4 = (short)var_4 >> 7;
	if ((short)var_4 < 0x73 || (var_4 == 0x73 && (arg_0 & 0x7fffff) < 0x689769)) {
		return arg_0;
	}
	var_10 = (arg_0 & 0x7fffff) | 0x800000;
	var_c = var_4 - 0x7f;
	if (var_c == 0 || (var_c & 0x8000) != 0) {
		var_20 = 0x10 - (short)var_c;
		var_14 = var_10 >> (var_20 & 0xff);
		var_24 = 1 << (var_20 & 0xff);
		var_1c = (var_24 - 1) & var_10;
	} else {
		var_10 = var_10 << (var_c & 0xff);
		var_10 = var_10 % table1[8];
		if (var_10 > table1[9]) {
			var_28 = var_28 ^ 0x8000;
			var_10 = var_10 - table1[9];
		}
		if (((var_10 >> 16) & 0xffff) > 0xff) {
			var_10 = table1[9] - var_10;
		}
		var_14 = (var_10 >> 16) & 0xffff;
		var_1c = var_10 & 0xffff;
		var_20 = 0x10;
		var_24 = 1 << (var_20 & 0xff);
	}
	var_18 = table2[var_14];
	var_8 = sub_3a6b8f(var_24, 0);
	var_18 = sub_3a65dc(var_18, var_8);
	var_4 = (var_18 >> 16) & 0x7f80;
	var_4 = (short)var_4 >> 7;
	var_4 = var_4 - (var_20 & 0xffff);
	if (var_4 == 0 || (var_4 & 0x8000) != 0) {
		var_18 = var_28 << 16;
		return var_18;
	}
	if ((short)var_4 < 0xff) {
		var_18 = var_18 & 0x7fffff;
		var_18 = var_18 | ((var_28 | (var_4 << 7)) << 16);
		return var_18;
	}
	var_18 = 0x7fffffff;
	var_18 = var_18 | (var_28 << 16);
	return var_18;
}

unsigned long sub_3a62c9(unsigned long arg_0, unsigned long arg_4)
{
	unsigned short var_1c;
	unsigned short var_c;
	unsigned short var_8;
	unsigned short var_18, var_28;
	unsigned short var_24, var_20;
	unsigned long var_14;
	unsigned long var_4;
	unsigned long var_10;
	unsigned long *var_2c;
	unsigned long *var_30;
	var_c = (arg_0 >> 16) & 0x7f80;
	if (var_c == 0)
		return arg_4;
	var_1c = (arg_4 >> 16) & 0x7f80;
	if (var_1c == 0)
		return arg_0;
	var_c >>= 7;
	var_1c >>= 7;
	var_18 = (arg_0 >> 16) & 0x8000;
	var_24 = (arg_4 >> 16) & 0x8000;
	var_14 = var_c - var_1c;
	var_4 = (arg_0 & 0x7fffff) | 0x800000;
	var_10 = (arg_4 & 0x7fffff) | 0x800000;
	if ((long)var_14 < 0 || (var_14 == 0 && var_4 < var_10)) {
		var_14 = ~var_14 + 1;
		var_8 = var_1c;
		if ((long)var_14 >= 0x18 || var_1c == 0xff)
			return arg_4;
		var_20 = var_24;
		var_28 = var_18;
		var_2c = &var_10;
		var_4 = var_4 >> (var_14 & 0xff);
		var_30 = &var_4;
	} else {
		var_8 = var_c;
		if ((long)var_14 >= 0x18 || var_c == 0xff)
			return arg_0;
		var_20 = var_18;
		var_28 = var_24;
		var_2c = &var_4;
		var_10 = var_10 >> (var_14 & 0xff);
		var_30 = &var_10;
	}
	if (var_20 != var_28) {
		*var_30 = ~(*var_30) + 1;
	}
	*var_2c = *var_2c + *var_30;
	if (*var_2c == 0)
		return *var_2c;
	if ((*var_2c >> 16) & 0xff80) {
		while ((*var_2c >> 16) & 0xff00) {
			if ((short)var_8 >= 0x7ff) {
				*var_2c = ((var_20 | 0x7fff) << 16) | 0xffff;
				return *var_2c;
			}
			*var_2c >>= 1;
			*var_2c = *var_2c & 0x7fffffff;
			var_8 += 1;
		}
	} else {
		while (!((*var_2c >> 16) & 0x80)) {
			if ((short)var_8 <= 0) {
				*var_2c = (var_20 << 16);
				return *var_2c;
			}
			*var_2c <<= 1;
			var_8 -= 1;
		}
	}
	*var_2c = *var_2c & 0x7fffff;
	*var_2c |= ((var_20 | (var_8 << 7)) << 16);
	return *var_2c;
}

unsigned long sub_3a65b1(unsigned long arg_0, unsigned long arg_4)
{
	unsigned long var_8 = arg_4 ^ 0x80000000;
	return sub_3a62c9(arg_0, var_8);
}

unsigned long sub_3a6b2c(unsigned long arg_0)
{
	unsigned long var_4;
	unsigned long var_c = table1[10];
	if ((arg_0 & 0x7fffffff) > 0x43fe7811)
		return 0x3f800000;
	var_4 = sub_3a65b1(var_c, arg_0);
	return sub_3a6886(var_4);
}

/** @ingroup misc
 * Generates XOR key.
 * This function takes a input key and will generate a 16 byte
 * XOR key that can be used to encrypt or decrypt add-on dictionaries.
 * @param[in] key input key used to generate XOR key
 * @param[in] size size of input key
 * @param[out] xorkey generated key
 */
void get_xor_key(char *key, long size, char *xorkey)
{
	unsigned long var_68, var_58, var_54, var_c, var_8, var_28, var_24, var_34;
	unsigned long var_1c, var_2c, var_64, var_6c, var_10, var_48, var_20, var_5c;
	unsigned long var_4, var_14, var_18, var_30, var_4c, var_50;
	char var_44[16];
	char *var_70 = key;
	char *var_60 = xorkey;
	var_1c = var_2c = 0;
	memset(var_44, 0, 16);
	var_68 = table1[0];
	var_58 = table1[1];
	var_54 = table1[2];
	var_c = table1[3];
	var_8 = table1[4];
	var_28 = table1[5];
	var_24 = table1[6];
	var_34 = table1[7];
	if (size >= 16) {
		memcpy(var_44, key, 16);
	} else {
		memcpy(var_44, key, size);
	}
	var_64 = 0;
	while(var_64 < size) {
		var_1c += *var_70;
		var_70++;
		var_64++;
	}
	var_6c = (var_1c & 0x1f) + 0x10;
	var_4 = sub_3a6b8f(var_44[0], 8);
	var_58 = sub_3a62c9(var_58, var_4);
	var_4 = sub_3a6b8f(var_44[1], 8);
	var_54 = sub_3a65b1(var_54, var_4);
	var_4 = sub_3a6b8f(var_44[2], 8);
	var_c = sub_3a62c9(var_c, var_4);
	var_4 = sub_3a6b8f(var_44[3], 8);
	var_8 = sub_3a65b1(var_8, var_4);
	var_4 = sub_3a6b8f(var_44[4], 8);
	var_28 = sub_3a62c9(var_28, var_4);
	var_4 = sub_3a6b8f(var_44[5], 8);
	var_24 = sub_3a65b1(var_24, var_4);
	var_64 = 0;
	while (var_64 < var_6c) {
		var_10 = sub_3a65dc(var_28, var_c);
		var_20 = sub_3a62c9(var_10, var_8);
		var_48 = sub_3a62c9(var_20, var_24);
		if (var_2c == size)
			var_2c = 0;
		var_5c = var_44[var_2c] ^ (var_c & 0xff);
		var_5c = (long)var_5c * ((long)(var_48 & 0xff));
		var_2c += 1;
		var_4 = sub_3a6b8f(var_5c, 8);
		var_48 = sub_3a62c9(var_48, var_4);
		var_18 = sub_3a6b2c(var_48);
		var_14 = sub_3a6886(var_48);
		var_10 = sub_3a65dc(var_18, var_c);
		var_20 = sub_3a65dc(var_14, var_8);
		var_30 = sub_3a65b1(var_10, var_20);
		var_30 = sub_3a65dc(var_68, var_30);
		var_4c = sub_3a62c9(var_30, var_58);
		var_10 = sub_3a65dc(var_14, var_c);
		var_20 = sub_3a65dc(var_18, var_8);
		var_30 = sub_3a62c9(var_10, var_20);
		var_30 = sub_3a65dc(var_68, var_30);
		var_50 = sub_3a62c9(var_30, var_54);
		var_c = var_4c;
		var_8 = var_50;
		var_64 += 1;
	}
	var_64 = 0;
	while(var_64 < 4) {
		var_10 = sub_3a65dc(var_28, var_c);
		var_20 = sub_3a62c9(var_10, var_8);
		var_48 = sub_3a62c9(var_20, var_24);
		var_18 = sub_3a6b2c(var_48);
		var_14 = sub_3a6886(var_48);
		var_10 = sub_3a65dc(var_18, var_c);
		var_20 = sub_3a65dc(var_14, var_8);
		var_30 = sub_3a65b1(var_10, var_20);
		var_30 = sub_3a65dc(var_68, var_30);
		var_4c = sub_3a62c9(var_30, var_58);
		var_10 = sub_3a65dc(var_14, var_c);
		var_20 = sub_3a65dc(var_18, var_8);
		var_30 = sub_3a62c9(var_10, var_20);
		var_30 = sub_3a65dc(var_68, var_30);
		var_50 = sub_3a62c9(var_30, var_54);
		*var_60 = (var_4c >> 16) & 0xff;
		++var_60;
		*var_60 = (var_4c >> 8) & 0xff;
		++var_60;
		*var_60 = (var_50 >> 16) & 0xff;
		++var_60;
		*var_60 = (var_50 >> 8) & 0xff;
		++var_60;
		var_c = var_4c;
		var_8 = var_50;
		var_64 += 1;
	}
}

/** @ingroup misc
 * Encrypts/Decrypts data.
 * This function will encrypt or decrypt data using the specified XOR
 * key.
 * @param[in,out] data data to be encrypted
 * @param[in] size size of data
 * @param[in] key xor key
 */
void crypt_data(char *data, int size, char *key)
{
	int blks, leftover;
	int i;
	char *ptr;
	blks = size >> 4;
	leftover = size % 16;
	ptr = data;
	for (i = 0; i < blks; i++) {
		*((long *)ptr) = *((long *)ptr) ^ ((long *)key)[0];
		ptr += 4;
		*((long *)ptr) = *((long *)ptr) ^ ((long *)key)[1];
		ptr += 4;
		*((long *)ptr) = *((long *)ptr) ^ ((long *)key)[2];
		ptr += 4;
		*((long *)ptr) = *((long *)ptr) ^ ((long *)key)[3];
		ptr += 4;
	}
	for (i = 0; i < leftover; i++) {
		*ptr = *ptr ^ key[i];
		ptr++;
	}
}

