#include "StdAfx.h"
#include "TestHDFSerializePolyData.h"

#include <fstream>
#include <vtkPolyData.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>

#include <vtkIOInstantiator.h> /* req'd for vtkZLibDataCompressor to be 
                                  registered for vtkInstantiator used by
								  vtkXMLPolyDataReader/vtkXMLReader */

#include "Global.h"

void TestHDFSerializePolyData::testPolyDataReaderWriter(void)
{
#ifdef _WIN64
	const char POLYDATA_BINARY_VTP[] = "Test/polydata.Binary.64.vtp";
	const char ACTUAL_BINARY_VTP[]   = "Test/actual.Binary.64.vtp";
#else
	const char POLYDATA_BINARY_VTP[] = "Test/polydata.Binary.32.vtp";
	const char ACTUAL_BINARY_VTP[]   = "Test/actual.Binary.32.vtp";
#endif

	vtkXMLPolyDataReader *reader = 0;
	vtkXMLPolyDataWriter *writer = 0;
	try
	{
		reader = vtkXMLPolyDataReader::New();
		reader->SetFileName(POLYDATA_BINARY_VTP);
		reader->Update(); // force read

		// verify 
		CPPUNIT_ASSERT(reader->GetNumberOfPoints() == 792);
		CPPUNIT_ASSERT(reader->GetNumberOfCells()  == 1440);
		CPPUNIT_ASSERT(reader->GetNumberOfVerts()  == 0);
		CPPUNIT_ASSERT(reader->GetNumberOfLines()  == 0);
		CPPUNIT_ASSERT(reader->GetNumberOfStrips() == 0);
		CPPUNIT_ASSERT(reader->GetNumberOfPolys()  == 1440);

		vtkPolyData *output = reader->GetOutput();
		CPPUNIT_ASSERT(output);

		writer = vtkXMLPolyDataWriter::New();
		writer->SetInput(output);
		writer->SetDataModeToBinary();
		writer->SetFileName(ACTUAL_BINARY_VTP);
		CPPUNIT_ASSERT(writer->Write() == 1);

		// clean-up
		reader->Delete();
		reader = 0;

		writer->Delete();
		writer = 0;
	}
	catch (...)
	{
		if (reader) reader->Delete();
		if (writer) writer->Delete();
		throw;
	}

	std::ifstream expected(POLYDATA_BINARY_VTP);
	CPPUNIT_ASSERT(expected.is_open());

	std::ifstream actual(ACTUAL_BINARY_VTP);
	CPPUNIT_ASSERT(actual.is_open());

	std::string str_expected;
	std::string str_actual;
	while(!(expected.eof()) && !(actual.eof()))
	{
		std::getline(expected, str_expected, '\n');
		std::getline(actual,   str_actual,   '\n');

#if defined(SKIP)
		::OutputDebugString("Expected:\n");
		::OutputDebugString(str_expected.c_str());
		::OutputDebugString("\n");

		::OutputDebugString("Actual:\n");
		::OutputDebugString(str_actual.c_str());
		::OutputDebugString("\n");
#endif
		CPPUNIT_ASSERT( str_expected == str_actual );
	}
	CPPUNIT_ASSERT( expected.eof() && actual.eof() );
	actual.close();
	CPPUNIT_ASSERT( ::remove(ACTUAL_BINARY_VTP) == 0 );
}

void TestHDFSerializePolyData::testHDF(void)
{
#ifdef _WIN64
	const char POLYDATA_BINARY_VTP[] = "Test/polydata.Binary.64.vtp";
	const char ACTUAL_BINARY_VTP[]   = "Test/actual.Binary.64.vtp";
	const char POLYDATA_H5[]         = "Test/polydata.64.h5";
#else
	const char POLYDATA_BINARY_VTP[] = "Test/polydata.Binary.32.vtp";
	const char ACTUAL_BINARY_VTP[]   = "Test/actual.Binary.32.vtp";
	const char POLYDATA_H5[]         = "Test/polydata.32.h5";
#endif
	const char DESERIALIZED_VTP[]    = "Test/deserialized.Binary.vtp";

	vtkXMLPolyDataReader *reader = 0;
	vtkXMLPolyDataWriter *writer = 0;
	vtkPolyData *deserialized = 0;

	try
	{
		reader = vtkXMLPolyDataReader::New();
		reader->SetFileName(POLYDATA_BINARY_VTP);
		reader->Update(); // force read

		// verify 
		CPPUNIT_ASSERT(reader->GetNumberOfPoints() == 792);
		CPPUNIT_ASSERT(reader->GetNumberOfCells()  == 1440);
		CPPUNIT_ASSERT(reader->GetNumberOfVerts()  == 0);
		CPPUNIT_ASSERT(reader->GetNumberOfLines()  == 0);
		CPPUNIT_ASSERT(reader->GetNumberOfStrips() == 0);
		CPPUNIT_ASSERT(reader->GetNumberOfPolys()  == 1440);

		vtkPolyData *output = reader->GetOutput();
		CPPUNIT_ASSERT(output);

		hid_t file_id = H5Fcreate(POLYDATA_H5, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
		CPPUNIT_ASSERT(file_id > 0);

		herr_t e = CGlobal::HDFSerializePolyData(true, file_id, "PolyData", output);
		CPPUNIT_ASSERT(e >= 0);

		herr_t s = H5Fclose(file_id);
		CPPUNIT_ASSERT(s >= 0);


		/* Create a new file using default properties. */
		file_id = H5Fopen(POLYDATA_H5, H5F_ACC_RDONLY, H5P_DEFAULT);
		CPPUNIT_ASSERT(file_id > 0);

		e = CGlobal::HDFSerializePolyData(false, file_id, "PolyData", deserialized);
		CPPUNIT_ASSERT(e >= 0);

		CPPUNIT_ASSERT(deserialized);

		writer = vtkXMLPolyDataWriter::New();
		writer->SetInput(deserialized);
		writer->SetDataModeToBinary();
		writer->SetFileName(DESERIALIZED_VTP);
		CPPUNIT_ASSERT(writer->Write() == 1);

		// clean-up
		reader->Delete();
		reader = 0;

		writer->Delete();
		writer = 0;

		deserialized->Delete();
		deserialized = 0;

	}
	catch (...)
	{
		if (reader) reader->Delete();
		if (writer) writer->Delete();
		if (deserialized) deserialized->Delete();
		throw;
	}

	std::ifstream expected(POLYDATA_BINARY_VTP);
	CPPUNIT_ASSERT(expected.is_open());

	std::ifstream actual(DESERIALIZED_VTP);
	CPPUNIT_ASSERT(actual.is_open());

	std::string str_expected;
	std::string str_actual;
	while(!(expected.eof()) && !(actual.eof()))
	{
		std::getline(expected, str_expected, '\n');
		std::getline(actual,   str_actual,   '\n');

#if defined(SKIP)
		::OutputDebugString("Expected:\n");
		::OutputDebugString(str_expected.c_str());
		::OutputDebugString("\n");

		::OutputDebugString("Actual:\n");
		::OutputDebugString(str_actual.c_str());
		::OutputDebugString("\n");
#endif
		CPPUNIT_ASSERT( str_expected == str_actual );
	}
	CPPUNIT_ASSERT( expected.eof() && actual.eof() );
	actual.close();
	CPPUNIT_ASSERT( ::remove(DESERIALIZED_VTP) == 0 );
}

#ifdef _WIN64
void TestHDFSerializePolyData::testReadHDF32As64(void)
{
	vtkXMLPolyDataWriter *writer = 0;
	vtkPolyData *deserialized = 0;

	try
	{
		hid_t file_id = H5Fopen("Test/polydata.32.h5", H5F_ACC_RDONLY, H5P_DEFAULT);
		CPPUNIT_ASSERT(file_id > 0);

		herr_t e = CGlobal::HDFSerializePolyData(false, file_id, "PolyData", deserialized);
		CPPUNIT_ASSERT(e >= 0);

		CPPUNIT_ASSERT(deserialized);

		writer = vtkXMLPolyDataWriter::New();
		writer->SetInput(deserialized);
		writer->SetDataModeToBinary();
		writer->SetFileName("Test/32as64.Binary.vtp");
		CPPUNIT_ASSERT(writer->Write() == 1);

		writer->Delete();
		writer = 0;

		deserialized->Delete();
		deserialized = 0;

	}
	catch (...)
	{
		if (writer) writer->Delete();
		if (deserialized) deserialized->Delete();
		throw;
	}

	std::ifstream expected("Test/polydata.Binary.64.vtp");
	CPPUNIT_ASSERT(expected.is_open());

	std::ifstream actual("Test/32as64.Binary.vtp");
	CPPUNIT_ASSERT(actual.is_open());

	std::string str_expected;
	std::string str_actual;
	while(!(expected.eof()) && !(actual.eof()))
	{
		std::getline(expected, str_expected, '\n');
		std::getline(actual,   str_actual,   '\n');

#if defined(SKIP)
		::OutputDebugString("Expected:\n");
		::OutputDebugString(str_expected.c_str());
		::OutputDebugString("\n");

		::OutputDebugString("Actual:\n");
		::OutputDebugString(str_actual.c_str());
		::OutputDebugString("\n");
#endif
		CPPUNIT_ASSERT( str_expected == str_actual );
	}
	CPPUNIT_ASSERT( expected.eof() && actual.eof() );
	actual.close();
	CPPUNIT_ASSERT( ::remove("Test/32as64.Binary.vtp") == 0 );
}
#else
void TestHDFSerializePolyData::testReadHDF64As32(void)
{
	vtkXMLPolyDataWriter *writer = 0;
	vtkPolyData *deserialized = 0;

	try
	{
		hid_t file_id = H5Fopen("Test/polydata.64.h5", H5F_ACC_RDONLY, H5P_DEFAULT);
		CPPUNIT_ASSERT(file_id > 0);

		herr_t e = CGlobal::HDFSerializePolyData(false, file_id, "PolyData", deserialized);
		CPPUNIT_ASSERT(e >= 0);

		CPPUNIT_ASSERT(deserialized);

		writer = vtkXMLPolyDataWriter::New();
		writer->SetInput(deserialized);
		writer->SetDataModeToBinary();
		writer->SetFileName("Test/64as32.Binary.vtp");
		CPPUNIT_ASSERT(writer->Write() == 1);

		writer->Delete();
		writer = 0;

		deserialized->Delete();
		deserialized = 0;
	}
	catch (...)
	{
		if (writer) writer->Delete();
		if (deserialized) deserialized->Delete();
		throw;
	}

	std::ifstream expected("Test/polydata.Binary.32.vtp");
	CPPUNIT_ASSERT(expected.is_open());

	std::ifstream actual("Test/64as32.Binary.vtp");
	CPPUNIT_ASSERT(actual.is_open());

	std::string str_expected;
	std::string str_actual;
	while(!(expected.eof()) && !(actual.eof()))
	{
		std::getline(expected, str_expected, '\n');
		std::getline(actual,   str_actual,   '\n');

#if defined(SKIP)
		::OutputDebugString("Expected:\n");
		::OutputDebugString(str_expected.c_str());
		::OutputDebugString("\n");

		::OutputDebugString("Actual:\n");
		::OutputDebugString(str_actual.c_str());
		::OutputDebugString("\n");
#endif
		CPPUNIT_ASSERT( str_expected == str_actual );
	}
	CPPUNIT_ASSERT( expected.eof() && actual.eof() );
	actual.close();
	CPPUNIT_ASSERT( ::remove("Test/64as32.Binary.vtp") == 0 );
}
#endif
