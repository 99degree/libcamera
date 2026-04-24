PipelineHandlerSoftISP::~PipelineHandlerSoftISP()
{
    LOG(SoftISPPipeline, Info) << "SoftISP pipeline handler destroyed";
    resetCreated_ = false;
}
