
#include <q/execution_context.hpp>
#include <q/blocking_dispatcher.hpp>
#include <q/threadpool.hpp>
#include <q/promise.hpp>

#include <gtest/gtest.h>

TEST( ExecutionContext, BlockDispatcher )
{
	auto bd = q::make_shared< q::blocking_dispatcher >( "test dispatcher" );
	auto ctx = q::make_shared< q::execution_context >( bd );

	auto qu = ctx->queue( );

	int num = 4711;
	q::with( qu, num )
	.then( [ ]( int i )
	{
		return i + 1;
	}, qu )
	.then( [ num, bd ]( int i )
	{
		EXPECT_EQ( i, num + 1 );
		bd->terminate( q::termination::linger );
	}, qu );

	bd->start( );
}

TEST( ExecutionContext, ThreadPoolDispatcher )
{
	auto bd = q::make_shared< q::blocking_dispatcher >( "test dispatcher" );
	auto ctx = q::make_shared< q::execution_context >( bd );
	auto qu = ctx->queue( );

	auto tpctx = q::make_execution_context< q::threadpool >( "test pool", qu );
	auto tpqu = tpctx->queue( );

	int num = 4711;
	std::atomic< int > val( num );

	std::size_t tasks = 1000;
	auto prom = q::with( tpqu, num ).share( );
	std::vector< q::promise< std::tuple< > > > promises;

	for ( std::size_t i = 0; i < tasks; ++i )
		promises.push_back(
			prom.then( [ &val ]( int i )
			{
				val += 1;
			}, tpqu )
		);

	q::all( std::move( promises ), tpqu )
	.then( [ num, &val, tasks, bd, tpctx ]( )
	{
		EXPECT_EQ( val.load( ), num + tasks );
		bd->terminate( q::termination::linger );
		tpctx->dispatcher( )->terminate( q::termination::linger );
	}, qu );

	bd->start( );
	tpctx->dispatcher( )->await_termination( );
}
