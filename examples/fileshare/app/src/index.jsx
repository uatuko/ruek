/* @refresh reload */
import { createSignal, lazy } from 'solid-js';
import { render } from 'solid-js/web';
import { Router, Route } from '@solidjs/router';

import './index.css';
import App from './app';

const File = lazy(() => import('./file'));
const SignUp = lazy(() => import('./sign-up'));
const Users = lazy(() => import('./users'));

const [user, setUser] = createSignal({});
const root = document.getElementById('root');

render(
	() => (
		<Router>
			<Route path="/" component={() => (<App user={user()} />)} />
			<Route path="/files/:id" component={() => (<File user={user()} />)} />
			<Route path="/sign-up" component={() => (<SignUp setUser={setUser} />)} />
			<Route path="/users" component={Users} />
		</Router>
	),
	root
);
