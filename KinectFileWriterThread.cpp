#include "KinectFileWriterThread.h"
#include <pcl/io/file_io.h>
#include <pcl/io/ply_io.h>
#include <pcl/io/pcd_io.h>
#include "stdafx.h"

template KinectFileWriterThread < pcl::PointXYZRGB >;
template KinectFileWriterThread < pcl::PointXYZ >;



template < class PointCloudType >
KinectFileWriterThread< PointCloudType >::KinectFileWriterThread()
{
}

template < class PointCloudType >
KinectFileWriterThread< PointCloudType >::~KinectFileWriterThread()
{
}


//
template < typename PointCloudType >
//void KinectFileWriterThread<PointCloudType>::writeCloudToFile(int index, RecordingFileFormat recordingFileFormat, std::string filePath, std::string fileName)
void KinectFileWriterThread<PointCloudType>::writeCloudToFile(int index, RecordingConfigurationPtr recordingConfiguration)
{
	auto recordingFileFormat =	recordingConfiguration->getRecordFileFormat();
	auto filePath =				recordingConfiguration->getFullRecordingPathString();
	auto fileName =				recordingConfiguration->getFileNameString();

	bool cloudIsEmpty = false;
	std::shared_ptr<pcl::FileWriter> writer;
	std::string fileFormatExtension;
	bool isBinary = false;
	switch (recordingFileFormat)
	{
	case PLY:
		writer = std::shared_ptr<pcl::PLYWriter> (new pcl::PLYWriter());
		fileFormatExtension = ".ply";
		isBinary = false;
		break;
	case PLY_BINARY:
		writer = std::shared_ptr<pcl::PLYWriter>(new pcl::PLYWriter());
		fileFormatExtension = ".ply";
		isBinary = true;
		break;
	case PCD:
		writer = std::shared_ptr<pcl::PCDWriter>(new pcl::PCDWriter());
		fileFormatExtension = ".pcd";
		isBinary = false;
		break;
	case PCD_BINARY:
		writer = std::shared_ptr<pcl::PCDWriter>(new pcl::PCDWriter());
		fileFormatExtension = ".pcd";
		isBinary = true;
		break;
	case RECORD_FILE_FORMAT_COUNT:
		break;
	default:
		break;
	}
	
	while(true){

		PointCloudMeasurement< PointCloudType > measurement;
		measurement.cloud = nullptr;
		measurement.index = 0;
		bool isDone = m_source->pullData(measurement);
		if (!measurement.cloud){
			OutputDebugString(L"writer finished cloud null");
			return;
		}
		
		std::stringstream outputFileWithPath;

		outputFileWithPath << filePath << "\\" << fileName << "_Cloud_" << measurement.index << fileFormatExtension;

		writer->write(outputFileWithPath.str(), *measurement.cloud, isBinary);

		if (isDone){
			OutputDebugString(L"writer finished isDone");
			return;
		}

	} 
}

template < typename PointCloudType >
void KinectFileWriterThread<PointCloudType>::setKinectCloudOutputWriter(CloudMeasurementSource<PointCloudType>* source)
{
	m_source = source;
}