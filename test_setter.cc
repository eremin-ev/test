/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>
#include <string>
#include <cassert>

struct MyDataBackend {
	unsigned char version[0x4];
	unsigned char array1[0x16];
	unsigned char value1;
	unsigned char value2;
	unsigned char value3;
	unsigned char value4;
	unsigned char reserved1[0x8];
	unsigned char value5;
	unsigned char value6;
	unsigned char value7;
	unsigned char value8;
	unsigned char reserved2[0x16];
	unsigned char array2[0x8];

	void f(int n)
	{
		printf("%s n %i\n", __PRETTY_FUNCTION__, n);
	}
};

struct MyData {
	enum Mode {
		ReadOnly,
		ReadWrite
	};

	MyData() :
		m_data(nullptr),
		m_mode(ReadOnly),
		m_changed(false)
	{
		printf("%s this %p\n", __PRETTY_FUNCTION__, this);
	}

	~MyData()
	{
		printf("%s m_data %p\n", __PRETTY_FUNCTION__, m_data);
		if (m_data)
			delete m_data;
	}

	bool isOpen() const { return m_data != nullptr; }

	bool isChanged() const { return m_changed == true; }

	bool open(Mode mode = ReadOnly)
	{
		if (m_data) {
			printf("%s leaking m_data %p\n", __PRETTY_FUNCTION__, m_data);
		}
		m_data = new MyDataBackend;
		m_mode = mode;
		printf("%s m_data %p, m_mode %i\n", __PRETTY_FUNCTION__, m_data, m_mode);
		return true;
	}

	std::string value(unsigned char MyDataBackend::*member) const
	{
		assert(m_data);

		const std::string &value = std::to_string(m_data->*member != 255 ? int(m_data->*member) : -1);

		printf("%s this %p, value %s\n", __PRETTY_FUNCTION__, this, value.c_str());

		return value;
	}

	void setValue(unsigned char MyDataBackend::*member, int value)
	{
		assert(!m_data || m_mode == ReadWrite);
		if (!isOpen() && !open(ReadWrite)) {
			return;
		}

		printf("%s this %p, value %i (origin %i)\n", __PRETTY_FUNCTION__, this,
			0xff & value, value);

		m_data->*member = value;
	}

private:
	MyDataBackend *m_data;
	Mode m_mode;
	bool m_changed;
};

int main(void)
{
	void (MyDataBackend::*pf)(int) = &MyDataBackend::f;

	MyData c, d;

	/* pointer-to-member is more like offset within a struct than like a pointer */
        unsigned char MyDataBackend::*data1 = &MyDataBackend::value1;

	//printf("data1 %i\n", static_cast<int>(data1));
	//int diff = data5 - data1;
	//printf("diff %i\n", static_cast<int>(data5 - data1));

	c.setValue(&MyDataBackend::value5, 515);

	std::cout << "data1: " << data1 << std::endl;
	std::cout << "data3: " << &MyDataBackend::value3 << std::endl;

	d.setValue(data1, 111);
	d.setValue(&MyDataBackend::value3, 113);

	c.value(&MyDataBackend::value5);

	d.value(data1);
	d.value(&MyDataBackend::value3);

	MyDataBackend db;
	MyDataBackend *dbp = &db;

	(db.*pf)(3);
	(dbp->*pf)(4);

	return 0;
}
