#include <nan.h>

#ifndef __FF_GENERICASYNCWORKER_H__
#define __FF_GENERICASYNCWORKER_H__

template<class Context>
class GenericAsyncWorker : public Nan::AsyncWorker {
public:
	Context ctx;

	GenericAsyncWorker(
		Nan::Callback *callback,
		Context ctx
	) : Nan::AsyncWorker(callback), ctx(ctx) {}
	~GenericAsyncWorker() {}

	void Execute() {
		std::string err = ctx.execute();
		if (!std::string(err).empty()) {
			this->SetErrorMessage(err.c_str());
		}
	}

	void HandleOKCallback() {
		Nan::HandleScope scope;
		v8::Local<v8::Value> argv[] = { Nan::Null(), ctx.getReturnValue() };
		callback->Call(2, argv);
	}

	void HandleErrorCallback() {
		Nan::HandleScope scope;
		v8::Local<v8::Value> argv[] = { Nan::New(this->ErrorMessage()).ToLocalChecked(), Nan::Null() };
		callback->Call(2, argv);
	}
};

struct SimpleWorker {
public:
	std::string execute() {
		return "";
	}

	v8::Local<v8::Value> getReturnValue() {
		return Nan::Undefined();
	}

	bool unwrapOptionalArgs(Nan::NAN_METHOD_ARGS_TYPE info) {
		return false;
	}

	bool hasOptArgsObject(Nan::NAN_METHOD_ARGS_TYPE info) {
		return false;
	}

	bool unwrapOptionalArgsFromOpts(Nan::NAN_METHOD_ARGS_TYPE info) {
		return false;
	}

	bool unwrapRequiredArgs(Nan::NAN_METHOD_ARGS_TYPE info) {
		return false;
	}
};

#define FF_RETHROW(ff_err) \
	tryCatch.Reset();\
	Nan::ThrowError(ff_err);\
	tryCatch.ReThrow();\
	return;\

#define FF_TRY_UNWRAP_ARGS(ff_methodName, applyUnwrappers)\
	Nan::TryCatch tryCatch;\
	if (applyUnwrappers) {\
		std::string err = std::string(*Nan::Utf8String(tryCatch.Exception()->ToString()));\
		FF_RETHROW(\
			Nan::New(\
				std::string(ff_methodName)\
				+ std::string(" - ")\
				+ err\
			).ToLocalChecked()\
		);\
	}

#define FF_WORKER_TRY_UNWRAP_ARGS(ff_methodName, ff_worker)\
	FF_TRY_UNWRAP_ARGS(\
		ff_methodName,\
		ff_worker.unwrapRequiredArgs(info) || \
		(ff_worker.hasOptArgsObject(info) && ff_worker.unwrapOptionalArgsFromOpts(info)) || \
		(!ff_worker.hasOptArgsObject(info) && ff_worker.unwrapOptionalArgs(info))\
	)

#define FF_WORKER_SYNC(ff_methodName, ff_worker)\
	FF_WORKER_TRY_UNWRAP_ARGS(ff_methodName, ff_worker);\
	std::string err = ff_worker.execute();\
	if (!err.empty()) {\
		tryCatch.Reset();\
		Nan::ThrowError(\
			Nan::New(\
				std::string(ff_methodName)\
				+ err\
			).ToLocalChecked()\
		);\
		tryCatch.ReThrow();\
		return;\
	}

#define FF_WORKER_ASYNC(ff_methodName, ff_Worker, ff_worker)\
	FF_WORKER_TRY_UNWRAP_ARGS(ff_methodName, ff_worker);\
	if (!hasArg(info.Length() - 1, info) || !info[info.Length() - 1]->IsFunction()) {\
		tryCatch.Reset();\
		Nan::ThrowError(\
			Nan::New(\
				std::string(ff_methodName)\
				+ std::string(" - ")\
				+ std::string(" callback function required")\
			).ToLocalChecked()\
		);\
		tryCatch.ReThrow();\
		return;\
	}\
	Nan::AsyncQueueWorker(new GenericAsyncWorker<ff_Worker>(\
		new Nan::Callback(info[info.Length() - 1].As<v8::Function>()),\
		ff_worker\
	));

#endif